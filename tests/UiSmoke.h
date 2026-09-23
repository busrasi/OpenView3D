#pragma once
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QQuickWindow>
#include <QImage>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include "ui/AppController.h"
#include "ai/ChatController.h"

// Available only in BUILD_TESTING builds. Exercises the actual QML submit function,
// provider, service and viewport bindings, then exits for repeatable GUI smoke tests.
inline void startAiSmokeTest(QGuiApplication& app, QQmlApplicationEngine& engine,
                             AppController& controller, ChatController& chat) {
    const int flag = app.arguments().indexOf("--ai-smoke-test");
    if (flag + 1 >= app.arguments().size() || engine.rootObjects().isEmpty()) {
        QTimer::singleShot(0, &app, [] { QCoreApplication::exit(2); });
        return;
    }
    const QString path = QFileInfo(app.arguments()[flag+1]).absoluteFilePath();
    auto* root = engine.rootObjects().first();
    auto* panel = root->findChild<QObject*>("aiChatPanel");
    auto* input = root->findChild<QObject*>("aiChatInput");
    if (!panel || !input) {
        qCritical() << "SMOKE FAIL: chat panel missing";
        QTimer::singleShot(0, &app, [] { QCoreApplication::exit(3); });
        return;
    }
    controller.loadModel(path);
    controller.setZoom(2.5f);
    QObject::connect(&chat, &ChatController::stateChanged, &app, [&app, &controller, &chat, root, path] {
        if (chat.busy()) return;
        if (!chat.error().isEmpty() || controller.modelPath() == path) {
            qCritical() << "SMOKE FAIL:" << chat.error();
            app.exit(4);
            return;
        }
        QTimer::singleShot(1500, &app, [&app, root, &controller] {
            auto* window = qobject_cast<QQuickWindow*>(root);
            const QString screenshot = QFileInfo(controller.modelPath()).dir().filePath("ai-smoke.png");
            if (window) window->grabWindow().save(screenshot);
            qInfo() << "SMOKE PASS: QML submitted, mock edited, selected OBJ updated:" << controller.modelPath();
            app.exit(0);
        });
    });
    QTimer::singleShot(1000, &app, [panel, input] {
        input->setProperty("text", "Make a hole through the center of the selected model.");
        QMetaObject::invokeMethod(panel, "submit");
    });
    QTimer::singleShot(30000, &app, [] { qCritical() << "SMOKE FAIL: timed out"; QCoreApplication::exit(5); });
}
