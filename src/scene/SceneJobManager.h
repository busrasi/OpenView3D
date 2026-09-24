#pragma once
#include "SceneData.h"
#include <QObject>
#include <QUuid>

struct SceneGenerationJob {
    QString id, originalPrompt, error, savedPath;
    SelectedModelSnapshot selectedModel;
    QJsonObject sceneSpec;
    QStringList generatedAssets;
    QString status = "Idle";
};
// One active job initially. Stable IDs and immutable snapshots allow future queues/history.
class SceneJobManager : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    SceneGenerationJob current;
    void begin(const QString& prompt, const SelectedModelSnapshot& hero) {
        current = {}; current.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        current.originalPrompt = prompt; current.selectedModel = hero; setStatus("UnderstandingRequest");
    }
    void setStatus(const QString& status) { current.status = status; emit changed(); }
signals:
    void changed();
};
