#include "ChatController.h"
#include "ui/AppController.h"
#include "mesh/MeshEditingService.h"
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

ChatController::ChatController(AppController* app, QObject* parent)
    : QObject(parent), m_app(app) {
    m_provider = qgetenv("OPENAI_API_KEY").trimmed().isEmpty()
        ? static_cast<AIProvider*>(new MockAIProvider(this)) : new OpenAIProvider(this);
    connect(app, &AppController::modelPathChanged, this, [this] {
        ++m_selectionRevision;
        emit selectedModelChanged();
    });
    connect(m_provider, &AIProvider::operationReady, this, &ChatController::execute);
    connect(m_provider, &AIProvider::answerReady, this, [this](const QString& text) { finish(text); });
    connect(m_provider, &AIProvider::failed, this, [this](const QString& error) { finish(error, true); });
    append("assistant", "Select an OBJ and ask me to make a hole through its center. The cut uses object-space Z; radius defaults to 10% of the smaller XY size. Your original file is kept.");
}
QString ChatController::selectedModelName() const {
    return QFileInfo(m_app->selectedModel()->modelPath).fileName();
}
void ChatController::append(const QString& role, const QString& text) {
    m_messages.append(QVariantMap{{"role", role}, {"text", text}});
    emit messagesChanged();
}
void ChatController::finish(const QString& text, bool failed) {
    m_busy = false;
    m_error = failed ? text : QString();
    append("assistant", (failed ? "Error: " : "") + text);
    emit stateChanged();
}
void ChatController::send(const QString& text) {
    if (m_busy || text.trimmed().isEmpty()) return;
    append("user", text.trimmed());
    if (text.size() > 4000) { finish("Please keep requests under 4000 characters.", true); return; }
    const auto* selected = m_app->selectedModel();
    if (!selected || selected->modelPath.isEmpty()) {
        finish("No model selected. Load an OBJ into the active view first.", true);
        return;
    }
    m_sourcePath = selected->modelPath;
    m_requestRevision = m_selectionRevision;
    m_busy = true;
    m_error.clear();
    emit stateChanged();
    m_provider->request(text.trimmed(), selectedModelName());
}
void ChatController::execute(const QJsonObject& command) {
    if (m_requestRevision != m_selectionRevision) {
        finish("Selection changed while AI was responding. Send the request again for the selected model.", true);
        return;
    }
    try {
        const auto operation = MeshOperation::fromJson(command);
        append("assistant", "Cutting a centered Z-axis through-hole in " + QFileInfo(m_sourcePath).fileName() + "...");
        auto* watcher = new QFutureWatcher<MeshEditResult>(this);
        connect(watcher, &QFutureWatcher<MeshEditResult>::finished, this, [this, watcher] {
            const auto result = watcher->result();
            watcher->deleteLater();
            if (!result.error.isEmpty()) { finish(result.error, true); return; }
            if (m_requestRevision != m_selectionRevision) {
                finish("Edited OBJ saved to " + result.outputPath + ". Selection changed during editing, so it was not loaded. Open it using Load Model.");
                return;
            }
            m_app->loadModel(result.outputPath);
            finish("Created a real through-hole and loaded " + result.outputPath + ". Original OBJ preserved.");
        });
        watcher->setFuture(QtConcurrent::run([path = m_sourcePath, operation] {
            return MeshEditingService::apply(path, operation);
        }));
    } catch (const std::exception& error) {
        finish(QString::fromUtf8(error.what()), true);
    }
}
