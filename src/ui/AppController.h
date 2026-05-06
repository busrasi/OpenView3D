#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "core/Renderer.h"

#include <QObject>
#include <QString>

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString texturePath READ texturePath NOTIFY texturePathChanged)

    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(float rotationX READ rotationX WRITE setRotationX NOTIFY rotationXChanged)
    Q_PROPERTY(float rotationY READ rotationY WRITE setRotationY NOTIFY rotationYChanged)

public:
    explicit AppController(QObject* parent = nullptr);

    QString modelPath() const;
    QString texturePath() const;

    float zoom() const;
    void setZoom(float value);

    float rotationX() const;
    void setRotationX(float value);

    float rotationY() const;
    void setRotationY(float value);

    Q_INVOKABLE void loadModel(const QString& path);
    Q_INVOKABLE void loadTexture(const QString& path);
    Q_INVOKABLE void resetCamera();

signals:
    void modelPathChanged();
    void texturePathChanged();
    void zoomChanged();
    void rotationXChanged();
    void rotationYChanged();

private:
    Renderer m_renderer;
};

#endif