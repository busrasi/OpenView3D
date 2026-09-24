#ifndef OPENGL_VIEWPORT_H
#define OPENGL_VIEWPORT_H

#include <QQuickFramebufferObject>
#include <QString>
#include <QVariant>
#include "core/MeshData.h"
#include "scene/SceneData.h"

class OpenGLViewport : public QQuickFramebufferObject
{
    Q_OBJECT

    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString texturePath READ texturePath WRITE setTexturePath NOTIFY texturePathChanged)

    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(float rotationX READ rotationX WRITE setRotationX NOTIFY rotationXChanged)
    Q_PROPERTY(float rotationY READ rotationY WRITE setRotationY NOTIFY rotationYChanged)

    Q_PROPERTY(float panX READ panX WRITE setPanX NOTIFY panXChanged)
    Q_PROPERTY(float panY READ panY WRITE setPanY NOTIFY panYChanged)

    Q_PROPERTY(QVariant meshData READ meshData WRITE setMeshData NOTIFY meshDataChanged)
    Q_PROPERTY(QVariant sceneData READ sceneData WRITE setSceneData NOTIFY sceneDataChanged)
public:
    QVariant meshData() const { return QVariant::fromValue(m_mesh); }
    QVariant sceneData() const { return QVariant::fromValue(m_scene); }
    void setMeshData(const QVariant& value) { m_mesh = value.value<MeshPtr>(); emit meshDataChanged(); update(); }
    void setSceneData(const QVariant& value) { m_scene = value.value<ScenePtr>(); emit sceneDataChanged(); update(); }
    explicit OpenGLViewport(QQuickItem* parent = nullptr);

    QString modelPath() const;
    void setModelPath(const QString& path);

    QString texturePath() const;
    void setTexturePath(const QString& path);

    float zoom() const;
    void setZoom(float value);

    float rotationX() const;
    void setRotationX(float value);

    float rotationY() const;
    void setRotationY(float value);

    float panX() const;
    void setPanX(float value);

    float panY() const;
    void setPanY(float value);

    QQuickFramebufferObject::Renderer* createRenderer() const override;

signals:
    void meshDataChanged();
    void sceneDataChanged();
    void renderingFailed(const QString& error);
    void modelPathChanged();
    void texturePathChanged();

    void zoomChanged();
    void rotationXChanged();
    void rotationYChanged();

    void panXChanged();
    void panYChanged();

private:
    MeshPtr m_mesh;
    ScenePtr m_scene;
    QString m_modelPath;
    QString m_texturePath;

    float m_zoom = 1.0f;
    float m_rotationX = 0.0f;
    float m_rotationY = 0.0f;

    float m_panX = 0.0f;
    float m_panY = 0.0f;
};

#endif
