#pragma once
#include "UiSmoke.h"
#include "ui/OpenGLViewport.h"
#include <QKeyEvent>
#include <QQuickItem>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QUrl>
#include <memory>

inline void startSceneSmokeTest(QGuiApplication& app, QQmlApplicationEngine& engine,
                               AppController& controller, ChatController& chat) {
    const int flag=app.arguments().indexOf("--scene-smoke-test");
    if (flag+2>=app.arguments().size() || engine.rootObjects().isEmpty()) { app.exit(2); return; }
    const QString inputPath=app.arguments()[flag+1];
    const bool bundled=inputPath.startsWith("qrc:");
    const QString resourcePath=bundled ? ":"+QUrl(inputPath).path() : QString();
    const QString path=bundled ? QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
        .filePath("bundled/"+QString::fromLatin1(QCryptographicHash::hash(resourcePath.toUtf8(),QCryptographicHash::Sha256).toHex().left(16))
        +"/"+QFileInfo(resourcePath).fileName()) : QFileInfo(inputPath).absoluteFilePath();
    const QString output=QFileInfo(app.arguments()[flag+2]).absoluteFilePath();
    QDir().mkpath(output);
    auto* root=engine.rootObjects().first();
    auto* window=qobject_cast<QQuickWindow*>(root);
    auto* panel=root->findChild<QObject*>("aiChatPanel");
    auto* input=root->findChild<QObject*>("aiChatInput");
    auto* viewport=root->findChild<OpenGLViewport*>("sceneViewport");
    if (!window||!panel||!input||!viewport) { QTimer::singleShot(0,&app,[]{QCoreApplication::exit(3);}); return; }
    struct State { int stage=0; MeshPtr mesh; ScenePtr first; QImage image; bool submitted=false; };
    auto state=std::make_shared<State>();
    const auto submit=[panel,input](const QString& text) {
        input->setProperty("text",text); QMetaObject::invokeMethod(panel,"submit");
    };
    QObject::connect(&controller,&AppController::renderingFailed,&app,[&app](const QString& e){qCritical()<<"SCENE SMOKE rendering failure:"<<e; app.exit(6);});
    QObject::connect(&controller,&AppController::meshChanged,&app,[&app,&controller,state,submit] {
        if (!controller.modelError().isEmpty()) { qCritical()<<controller.modelError(); app.exit(7); return; }
        if (state->submitted||!controller.selectedModel()->mesh) return;
        state->submitted=true; state->mesh=controller.selectedModel()->mesh;
        QTimer::singleShot(500,&app,[submit]{submit("Show this vase in a modern room on a wooden table.");});
    });
    QObject::connect(&chat,&ChatController::stateChanged,&app,[&app,&controller,&chat,state,path,output,window,viewport,submit] {
        if (chat.busy()||!state->submitted) return;
        const auto scene=controller.selectedModel()->scene;
        if (!chat.error().isEmpty()||!scene||controller.modelPath()!=path||controller.selectedModel()->mesh!=state->mesh
            ||chat.lastIntent()!="SCENE_GENERATION"||scene->objects.size()!=8||std::abs(scene->heroMinimum.y()-.8f)>.0001f) {
            qCritical()<<"SCENE SMOKE FAIL:"<<chat.error(); app.exit(4); return;
        }
        if (state->stage==0) {
            state->first=scene; state->stage=1;
            QTimer::singleShot(1000,&app,[&app,window,viewport,output,state,submit] {
                state->image=window->grabWindow();
                if (state->image.isNull()||!state->image.save(QDir(output).filePath("scene-modern.png"))) { app.exit(8); return; }
                // Verify presentation as well as mathematical contact: the blue-gray hero
                // must appear above the brown support, not upside-down in Qt's FBO.
                const auto origin=viewport->mapToScene(QPointF(0,0));
                const auto dpr=state->image.devicePixelRatio();
                const QRect area(int(origin.x()*dpr),int(origin.y()*dpr),int(viewport->width()*dpr),int(viewport->height()*dpr));
                const auto image=state->image.copy(area);
                qint64 heroY=0,woodY=0; int heroCount=0,woodCount=0;
                for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
                    const auto c=image.pixelColor(x,y);
                    if (c.blue()>c.red()*1.12 && c.green()>c.red()*1.05 && c.blue()<245) {heroY+=y;++heroCount;}
                    if (c.red()>c.green()*1.35 && c.green()>c.blue()*1.25 && c.red()>35) {woodY+=y;++woodCount;}
                }
                if (heroCount<100||woodCount<100||double(heroY)/heroCount>=double(woodY)/woodCount) {
                    qCritical()<<"SCENE SMOKE: rendered support/hero orientation is wrong";app.exit(13);return;
                }
                submit("Make the room more minimalist.");
            });
        } else if (state->stage==1) {
            state->stage=2;
            if (scene->id==state->first->id||scene->spec.style!="minimalist") { app.exit(9); return; }
            QTimer::singleShot(1000,&app,[&app,&controller,window,viewport,output,state] {
                window->grabWindow().save(QDir(output).filePath("scene-minimalist.png"));
                // Exercise the QML key handler and zoom property binding used by wheel/slider input.
                viewport->parentItem()->forceActiveFocus();
                QKeyEvent key(QEvent::KeyPress,Qt::Key_Right,Qt::NoModifier);
                QCoreApplication::sendEvent(window,&key);
                if (controller.rotationY()==0) { qCritical()<<"SCENE SMOKE: rotation key failed"; app.exit(10); return; }
                controller.setRotationY(65); controller.setZoom(.72f);
                QTimer::singleShot(1000,&app,[&app,&controller,window,viewport,output,state] {
                    const auto rotated=window->grabWindow();
                    if (rotated.isNull()||rotated==state->image||viewport->zoom()!=controller.zoom()
                        ||viewport->rotationY()!=controller.rotationY()) { app.exit(11); return; }
                    rotated.save(QDir(output).filePath("scene-orbit-zoom.png"));
                    controller.clearGeneratedScene();
                    if (controller.selectedModel()->mesh!=state->mesh||controller.selectedModel()->scene) { app.exit(12); return; }
                    qInfo()<<"SCENE SMOKE PASS: vase, QML submission, real geometry, support contact, lighting, camera, regeneration, orbit/zoom, clear, hero preserved";
                    app.exit(0);
                });
            });
        }
    });
    controller.loadModel(inputPath);
    QTimer::singleShot(30000,&app,[]{qCritical()<<"SCENE SMOKE timeout";QCoreApplication::exit(5);});
}
