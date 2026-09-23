#pragma once
#include <QObject>
#include <QVariantList>
#include "AIProvider.h"
class AppController;

class ChatController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QString selectedModelName READ selectedModelName NOTIFY selectedModelChanged)
    Q_PROPERTY(QString providerName READ providerName CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
public:
    explicit ChatController(AppController* app, QObject* parent = nullptr);
    QVariantList messages() const { return m_messages; }
    QString selectedModelName() const;
    QString providerName() const { return m_provider->name(); }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    Q_INVOKABLE void send(const QString& text);
signals:
    void messagesChanged();
    void selectedModelChanged();
    void stateChanged();
private:
    void append(const QString& role, const QString& text);
    void finish(const QString& text, bool failed = false);
    void execute(const QJsonObject& command);
    AppController* m_app;
    AIProvider* m_provider;
    QVariantList m_messages;
    bool m_busy = false;
    QString m_error;
    QString m_sourcePath;
    quint64 m_selectionRevision = 0;
    quint64 m_requestRevision = 0;
};
