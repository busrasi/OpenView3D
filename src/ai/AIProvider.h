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
signals:
    void operationReady(const QJsonObject& operation);
    void answerReady(const QString& text);
    void failed(const QString& error);
};

class MockAIProvider final : public AIProvider {
public:
    using AIProvider::AIProvider;
    QString name() const override { return "Mock AI (offline)"; }
    void request(const QString& text, const QString& modelName) override;
};

class OpenAIProvider final : public AIProvider {
public:
    explicit OpenAIProvider(QObject* parent = nullptr);
    QString name() const override { return "Real AI (OpenAI)"; }
    void request(const QString& text, const QString& modelName) override;
private:
    QNetworkAccessManager m_network;
};
