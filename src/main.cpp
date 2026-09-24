#include "ui/AppController.h"
#include "ai/ChatController.h"
#include "ui/OpenGLViewport.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QtQml>
#include <QIcon>
#ifdef OPENVIEW3D_TESTING
#include "tests/UiSmoke.h"
#include "tests/SceneUiSmoke.h"
#endif

int main(int argc, char* argv[])
{

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
#ifdef OPENVIEW3D_TESTING
    const bool aiSmoke = app.arguments().contains("--ai-smoke-test");
    const bool sceneSmoke = app.arguments().contains("--scene-smoke-test");
    if (aiSmoke || sceneSmoke) qunsetenv("OPENAI_API_KEY");
#endif
    app.setWindowIcon(
        QIcon(":/qt/qml/OpenView3D/qml/icons/icon256.png")
        );
    qmlRegisterType<OpenGLViewport>("OpenView3D", 1, 0, "OpenGLViewport");

    AppController appController;
    ChatController chatController(&appController);
    // Destroy QML bindings before the controllers they reference.
    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
        );

    engine.setInitialProperties({
        { "appController", QVariant::fromValue(&appController) },
        { "chatController", QVariant::fromValue(&chatController) }
    });

    engine.loadFromModule("OpenView3D", "Main");

#ifdef OPENVIEW3D_TESTING
    if (aiSmoke) startAiSmokeTest(app, engine, appController, chatController);
    if (sceneSmoke) startSceneSmokeTest(app, engine, appController, chatController);
#endif

    return app.exec();
}
