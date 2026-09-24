#include "core/loader.h"
#include "scene/SceneBuilder.h"
#include "scene/ScenePresets.h"
#include "scene/SceneGenerationController.h"
#include "ai/ChatController.h"
#include "ai/RequestRouter.h"
#include "ui/AppController.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QThread>
#include <QFile>
#include <QJsonDocument>
#include <iostream>
#include <functional>

static void check(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
static void wait(const std::function<bool()>& done) {
    QElapsedTimer t; t.start();
    while (!done() && t.elapsed() < 15000) { QCoreApplication::processEvents(); QThread::msleep(1); }
    check(done(),"Asynchronous operation timed out");
}
static void reject(QJsonObject json) {
    bool failed = false; try { SceneSpec::fromJson(json); } catch (...) { failed = true; }
    check(failed,"Untrusted scene was accepted");
}
int main(int argc, char** argv) {
    QCoreApplication app(argc,argv); app.setApplicationName("OpenView3D-SceneTests");
    QStandardPaths::setTestModeEnabled(true); qunsetenv("OPENAI_API_KEY");
    try {
        const auto json = SceneSpec{}.toJson();
        check(SceneSpec::fromJson(json).toJson() == json,"Spec round trip failed");
        for (auto it=json.begin();it!=json.end();++it) {
            auto bad=json; bad.remove(it.key()); reject(bad);
            bad=json; bad[it.key()]=false; reject(bad);
        }
        auto bad=json; bad["path"]="../../evil.obj"; reject(bad);
        for (double value : {-1.,0.,2.,21.,1e100}) { bad=json; auto room=bad["room"].toObject(); room["width"]=value; bad["room"]=room; reject(bad); }
        bad=json; bad["style"]="execute shell"; reject(bad);
        bad=json; auto surface=bad["surface"].toObject(); surface["material"]="remote_url"; bad["surface"]=surface; reject(bad);
        bad=json; auto hero=bad["heroModel"].toObject(); hero["source"]="arbitrary.obj"; bad["heroModel"]=hero; reject(bad);
        check(RequestRouter::route("Show this vase in a modern room on a wooden table.",false)==RequestRouter::SceneGeneration,"Scene route failed");
        check(RequestRouter::route("Make the room more minimalist.",true)==RequestRouter::SceneGeneration,"Follow-up route failed");
        check(RequestRouter::route("Make a hole through the center.",true)==RequestRouter::MeshEdit,"Mesh route failed");
        check(RequestRouter::route("What model is currently selected?",true)==RequestRouter::ModelQuery,"Query route failed");
        check(RequestRouter::route("Do not create a room",true)==RequestRouter::Unsupported,"Negation routed as mutation");
        SceneAssetManager assets;
        bool failed=false; try { assets.resolve("https://bad/asset.obj"); } catch (...) { failed=true; }
        check(failed,"Unknown asset accepted");
        struct InvalidAssetProvider : GeneratedAssetProvider {
            bool supports(const QString&) const override { return true; }
            SceneGeometryPtr resolve(const QString&) const override { return std::make_shared<SceneGeometry>(); }
        };
        SceneAssetManager brokenAssets; brokenAssets.addProvider(std::make_shared<InvalidAssetProvider>());
        failed=false; try { brokenAssets.resolve("unit_box"); } catch (...) { failed=true; }
        check(failed,"Invalid generated geometry accepted");
        SelectedModelSnapshot snapshot; snapshot.minimum={12,-7,22}; snapshot.maximum={14,13,26};
        for (const auto& support : {"table","pedestal"}) {
            SceneSpec s; s.surface=support;
            const auto scene=SceneBuilder::build(s,snapshot,"bounds-test",assets);
            check(std::abs(scene.heroMinimum.y()-(s.surface=="table"?.8f:1.05f))<1e-5f,"Hero is not on support");
            check(std::abs(scene.heroMinimum.x()+scene.heroMaximum.x())<1e-5f,"Hero not centered");
            check(scene.objects.size() == (s.surface=="table"?8:4),"Missing environment geometry");
            check(scene.heroMaximum.y()<s.height,"Hero exceeds room");
            for (auto preset : {"product_focus","wide_scene","close_product"}) {
                auto copy=scene; copy.spec.camera=preset; ScenePresets::camera(copy);
                check(copy.cameraRadius>0 && qIsFinite(copy.cameraRadius),"Invalid camera");
            }
        }
        QTemporaryDir temp; check(temp.isValid(),"Temporary directory unavailable");
        const auto path=temp.filePath(QString::fromUtf8("vase space ü.obj"));
        QFile source(path); check(source.open(QIODevice::WriteOnly),"Cannot create fixture");
        const QByteArray original="v 12 -7 22\nv 14 -7 22\nv 13 13 26\nf 1 2 3\n";
        source.write(original); source.close();
        AppController controller; ChatController chat(&controller);
        chat.send("Show this vase in a modern room on a wooden table.");
        check(chat.error().contains("No model selected"),"Missing selection not handled");
        controller.loadModel(path); wait([&]{return bool(controller.selectedModel()->mesh)||!controller.modelError().isEmpty();});
        check(controller.modelError().isEmpty(),"Unicode/spaced path load failed");
        check(controller.selectedModel()->mesh->normals.front().y!=0,"Missing OBJ normals were not generated");
        const auto mesh=controller.selectedModel()->mesh;
        QStringList statuses;
        QObject::connect(&chat,&ChatController::stateChanged,&app,[&]{statuses.append(chat.sceneStatus());});
        auto send=[&](QString prompt) { chat.send(prompt); wait([&]{return !chat.busy();}); check(chat.error().isEmpty(),qPrintable(chat.error())); };
        send("Show this vase in a modern room on a wooden table.");
        auto first=controller.selectedModel()->scene;
        check(bool(first)&&first->spec.style=="modern"&&first->spec.material=="wood","Modern scene failed");
        QFile saved(first->savedPath); check(saved.open(QIODevice::ReadOnly),"Scene not persisted");
        const auto persisted=QJsonDocument::fromJson(saved.readAll()).object();
        check(persisted["sceneSpec"].toObject()==first->spec.toJson(),"Persistence differs from validated spec");
        send("Make the room more minimalist.");
        auto second=controller.selectedModel()->scene;
        check(second->id!=first->id&&second->spec.style=="minimalist"&&second->objects.size()==8,"Regeneration did not replace environment");
        check(controller.modelPath()==path&&controller.selectedModel()->mesh==mesh,"Hero duplicated or replaced");
        send("Put this model on a pedestal in a minimalist room");
        check(controller.selectedModel()->scene->spec.surface=="pedestal","Pedestal mock failed");
        send("Create a cozy room around this model");
        check(controller.selectedModel()->scene->spec.lighting=="warm_interior","Cozy lighting failed");
        send("Make the room Scandinavian instead.");
        check(controller.selectedModel()->scene->spec.style=="scandinavian","Scandinavian regeneration failed");
        for (auto status : {"UnderstandingRequest","GeneratingScene","LoadingAssets","BuildingScene","Completed"})
            check(statuses.contains(status),"Missing job state");
        send("What model is currently selected?"); check(chat.lastIntent()=="MODEL_QUERY","Query intent missing");
        send("Clear generated scene"); check(!controller.selectedModel()->scene&&controller.selectedModel()->mesh==mesh,"Clear removed hero");
        chat.send("Create a cozy room around this model"); controller.addView();
        wait([&]{return !chat.busy();}); check(chat.error().contains("Selection changed"),"Stale result applied");
        controller.setActiveViewIndex(0);
        chat.send("Create a cozy room around this model");
        controller.clearGeneratedScene(); wait([&]{return !chat.busy();});
        check(!controller.selectedModel()->scene,"Clear during request was undone");
        check(source.open(QIODevice::ReadOnly),"Cannot reopen original"); check(source.readAll()==original,"Original OBJ modified");
        controller.loadModel(temp.filePath("missing.obj")); wait([&]{return !controller.modelError().isEmpty();});
        chat.send("Create a cozy room around this model"); check(!chat.error().isEmpty(),"Missing model not reported");
        std::cout << "PASS: strict validation, routing, bounds, geometry, presets, async jobs, Unicode paths, persistence, regeneration, selection safety, hero preservation\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
