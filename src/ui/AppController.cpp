#include "AppController.h"

#include <QDebug>

AppController::AppController(QObject* parent)
    : QObject(parent)
{
}

QString AppController::modelPath() const
{
    return m_renderer.modelPath();
}

QString AppController::texturePath() const
{
    return m_renderer.texturePath();
}

float AppController::zoom() const
{
    return m_renderer.zoom();
}

void AppController::setZoom(float value)
{
    if (qFuzzyCompare(m_renderer.zoom(), value)) {
        return;
    }

    m_renderer.setZoom(value);
    emit zoomChanged();
}

float AppController::rotationX() const
{
    return m_renderer.rotationX();
}

void AppController::setRotationX(float value)
{
    if (qFuzzyCompare(m_renderer.rotationX(), value)) {
        return;
    }

    m_renderer.setRotation(value, m_renderer.rotationY());
    emit rotationXChanged();
}

float AppController::rotationY() const
{
    return m_renderer.rotationY();
}

void AppController::setRotationY(float value)
{
    if (qFuzzyCompare(m_renderer.rotationY(), value)) {
        return;
    }

    m_renderer.setRotation(m_renderer.rotationX(), value);
    emit rotationYChanged();
}

void AppController::loadModel(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }

    const bool loaded = m_renderer.loadModel(path);

    if (!loaded) {
        qDebug() << "AppController failed to load model:" << path;
        return;
    }

    emit modelPathChanged();

    qDebug() << "AppController loaded model:" << path;
}

void AppController::loadTexture(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }

    const bool loaded = m_renderer.loadTexture(path);

    if (!loaded) {
        qDebug() << "AppController failed to store texture:" << path;
        return;
    }

    emit texturePathChanged();

    qDebug() << "AppController selected texture:" << path;
}

void AppController::resetCamera()
{
    m_renderer.resetCamera();

    emit zoomChanged();
    emit rotationXChanged();
    emit rotationYChanged();

    qDebug() << "AppController reset camera";
}