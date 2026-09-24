#pragma once
#include <QObject>
#include <QJsonObject>
#include <QNetworkAccessManager>

class AIProvider : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual QString name() const = 0;
    virtual void request(const QString& text, const QString& modelName) = 0;
    virtual void requestScene(const QString& text, const QString& modelName, const QJsonObject& previous) = 0;
signals:
    void sceneReady(const QJsonObject& scene);
    void operationReady(const QJsonObject& operation);
    void answerReady(const QString& text);
    void failed(const QString& error);
};

class MockAIProvider final : public AIProvider {
public:
    using AIProvider::AIProvider;
    QString name() const override { return "Mock AI (offline)"; }
    void request(const QString& text, const QString& modelName) override;
    void requestScene(const QString& text, const QString& modelName, const QJsonObject& previous) override;
};

class OpenAIProvider final : public AIProvider {
public:
    explicit OpenAIProvider(QObject* parent = nullptr);
    QString name() const override { return "Real AI (OpenAI)"; }
    void request(const QString& text, const QString& modelName) override;
    void requestScene(const QString& text, const QString& modelName, const QJsonObject& previous) override;
private:
    void postStructured(const QString& text, const QString& modelName, const QString& instruction, const QJsonObject& schema, bool scene);
    QNetworkAccessManager m_network;
};
