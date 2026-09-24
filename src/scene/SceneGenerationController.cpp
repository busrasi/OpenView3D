#include "SceneGenerationController.h"
#include "SceneBuilder.h"
#include "ui/AppController.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

SceneGenerationController::SceneGenerationController(AppController* app, QObject* parent)
    : QObject(parent), m_app(app) {
    m_provider = qgetenv("OPENAI_API_KEY").trimmed().isEmpty()
        ? static_cast<AIProvider*>(new MockAIProvider(this)) : new OpenAIProvider(this);
    connect(&m_jobs, &SceneJobManager::changed, this, &SceneGenerationController::stateChanged);
    connect(m_provider, &AIProvider::sceneReady, this, &SceneGenerationController::build);
    connect(m_provider, &AIProvider::failed, this, &SceneGenerationController::fail);
    connect(m_provider, &AIProvider::answerReady, this, &SceneGenerationController::fail);
}
void SceneGenerationController::fail(const QString& error) {
    m_jobs.current.error = error; m_jobs.setStatus("Failed"); emit finished(error,true);
}
void SceneGenerationController::start(const QString& prompt) {
    m_jobs.begin(prompt,m_app->selectedSnapshot());
    if (!m_app->selectedModel()->mesh) {
        fail(m_app->modelError().isEmpty() ? "Model is still loading. Try again when it appears in the viewport." : m_app->modelError()); return;
    }
    m_jobs.setStatus("GeneratingScene");
    const auto existing = m_app->selectedModel()->scene;
    m_provider->requestScene(prompt,m_jobs.current.selectedModel.name,existing ? existing->spec.toJson() : QJsonObject{});
}
void SceneGenerationController::build(const QJsonObject& json) {
    try {
        const auto spec = SceneSpec::fromJson(json);
        if (m_jobs.current.selectedModel.revision != m_app->selectionRevision()) {
            fail("Selection changed during scene generation. Send the request again."); return;
        }
        m_jobs.current.sceneSpec = spec.toJson();
        m_jobs.setStatus("LoadingAssets");
        // Providers and construction run on a pool thread; only the final pointer swap touches the view.
        struct Result { ScenePtr scene; QString error; };
        auto* watcher = new QFutureWatcher<Result>(this);
        connect(watcher,&QFutureWatcher<Result>::finished,this,[this,watcher] {
            const auto result = watcher->result(); watcher->deleteLater();
            if (!result.error.isEmpty()) { fail(result.error); return; }
            if (m_jobs.current.selectedModel.revision != m_app->selectionRevision()) {
                fail("Selection changed. Generated scene was saved but not applied: " + result.scene->savedPath); return;
            }
            m_jobs.current.savedPath = result.scene->savedPath;
            m_jobs.current.generatedAssets = result.scene->assets.keys();
            m_app->applyScene(result.scene);
            m_jobs.setStatus("Completed");
            emit finished("Created a real 3D " + result.scene->spec.style + " room with a " + result.scene->spec.surface
                + ". Rotate and zoom to inspect it. Scene saved to " + result.scene->savedPath,false);
        });
        m_jobs.setStatus("BuildingScene");
        watcher->setFuture(QtConcurrent::run([spec,hero=m_jobs.current.selectedModel,id=m_jobs.current.id] {
            Result r;
            try {
                SceneAssetManager assets;
                auto scene = std::make_shared<SceneData>(SceneBuilder::build(spec,hero,id,assets));
                scene->savedPath = SceneAssetManager::save(*scene,hero); r.scene = scene;
            } catch (const std::exception& e) { r.error = QString::fromUtf8(e.what()); }
            catch (...) { r.error = "Scene construction failed."; }
            return r;
        }));
    } catch (const std::exception& e) { fail(QString::fromUtf8(e.what())); }
}
