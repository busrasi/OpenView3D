#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>
#include "core/MeshData.h"
#include "scene/SceneData.h"
#include <QNetworkAccessManager>

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int activeViewIndex READ activeViewIndex WRITE setActiveViewIndex NOTIFY activeViewIndexChanged)
    Q_PROPERTY(int viewCount READ viewCount NOTIFY viewCountChanged)

    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString texturePath READ texturePath NOTIFY texturePathChanged)

    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(float rotationX READ rotationX WRITE setRotationX NOTIFY rotationXChanged)
    Q_PROPERTY(float rotationY READ rotationY WRITE setRotationY NOTIFY rotationYChanged)

public:
    Q_PROPERTY(QVariant meshData READ meshData NOTIFY meshChanged)
    Q_PROPERTY(QVariant sceneData READ sceneData NOTIFY sceneChanged)
    Q_PROPERTY(QString modelError READ modelError NOTIFY meshChanged)
    QVariant meshData() const { return QVariant::fromValue(activeView().mesh); }
    QVariant sceneData() const { return QVariant::fromValue(activeView().scene); }
    QString modelError() const { return activeView().loadError; }
    quint64 selectionRevision() const { return m_selectionRevision; }
    SelectedModelSnapshot selectedSnapshot() const;
    void applyScene(ScenePtr scene);
    Q_INVOKABLE void clearGeneratedScene();
    Q_INVOKABLE void reportRenderError(const QString& error) { emit renderingFailed(error); }
    explicit AppController(QObject* parent = nullptr);

    int activeViewIndex() const;
    void setActiveViewIndex(int index);

    int viewCount() const;

    QString modelPath() const;
    QString texturePath() const;

    float zoom() const;
    void setZoom(float value);

    float rotationX() const;
    void setRotationX(float value);

    float rotationY() const;
    void setRotationY(float value);

    Q_INVOKABLE bool addView();
    Q_INVOKABLE void loadModel(const QString& path);
    Q_INVOKABLE void loadTexture(const QString& path);
    Q_INVOKABLE void resetCamera();
    Q_INVOKABLE void closeView(int index);

    Q_INVOKABLE void generateModel(const QString& imagePath);

signals:
    void renderingFailed(const QString& error);
    void meshChanged();
    void sceneChanged();
    void activeViewIndexChanged();
    void viewCountChanged();

    void modelPathChanged();
    void texturePathChanged();
    void zoomChanged();
    void rotationXChanged();
    void rotationYChanged();

    void generationStarted();
    void modelGenerated(const QString& path);
    void generationFailed(const QString& error);

public:
    // The existing active view is the selected model record; no separate selection store.
    struct ViewState {
        MeshPtr mesh;
        ScenePtr scene;
        QString loadError;
        quint64 loadToken = 0;
        QString modelPath;
        QString texturePath;
        float zoom = 1.0f;
        float rotationX = 0.0f;
        float rotationY = 0.0f;
    };

    const ViewState* selectedModel() const { return &activeView(); }

private:

    ViewState& activeView();
    const ViewState& activeView() const;

private:
    quint64 m_selectionRevision = 0;
    quint64 m_loadToken = 0;
    QVector<ViewState> m_views;
    int m_activeViewIndex = 0;
    static constexpr int MaxViews = 15;

    QNetworkAccessManager* m_networkManager = nullptr;
};

#endif
