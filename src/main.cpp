#include "ui/AppController.h"
#include "ui/OpenGLViewport.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QtQml>

int main(int argc, char* argv[])
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);

    qmlRegisterType<OpenGLViewport>("OpenView3D", 1, 0, "OpenGLViewport");

    QQmlApplicationEngine engine;

    AppController appController;

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
        { "appController", QVariant::fromValue(&appController) }
    });

    engine.loadFromModule("OpenView3D", "Main");

    return app.exec();
}