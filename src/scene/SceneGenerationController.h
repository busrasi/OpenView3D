#pragma once
#include "SceneJobManager.h"
#include "ai/AIProvider.h"
class AppController;
class SceneGenerationController : public QObject {
    Q_OBJECT
public:
    SceneGenerationController(AppController* app, QObject* parent = nullptr);
    void start(const QString& prompt);
    QString status() const { return m_jobs.current.status; }
    const SceneGenerationJob& job() const { return m_jobs.current; }
signals:
    void stateChanged();
    void finished(const QString& text, bool failed);
private:
    void build(const QJsonObject& json);
    void fail(const QString& error);
    AppController* m_app;
    AIProvider* m_provider;
    SceneJobManager m_jobs;
};
