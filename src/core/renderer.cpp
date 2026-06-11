#include "Renderer.h"

#include <QDebug>
#include <QUrl>

namespace
{
QString toLocalFilePath(const QString& path)
{
    if (path.startsWith("file://")) {
        return QUrl(path).toLocalFile();
    }

    return path;
}
}

Renderer::Renderer()
{
    qDebug() << "Renderer created";
}

Renderer::~Renderer()
{
    qDebug() << "Renderer destroyed";
}

bool Renderer::loadModel(const QString& path)
{
    if (path.isEmpty()) {
        qDebug() << "Renderer loadModel failed: empty path";
        return false;
    }

    const QString localPath = toLocalFilePath(path);

    m_loader.vertices.clear();
    m_loader.uvs.clear();
    m_loader.normals.clear();

    const bool loaded = m_loader.loadOBJ(localPath.toUtf8().constData());

    if (!loaded) {
        qDebug() << "Renderer failed to load model:" << localPath;
        m_modelPath.clear();
        return false;
    }

    m_modelPath = localPath;

    qDebug() << "Renderer loaded model:" << m_modelPath;
    qDebug() << "Vertices:" << m_loader.vertices.size();
    qDebug() << "UVs:" << m_loader.uvs.size();
    qDebug() << "Normals:" << m_loader.normals.size();

    return true;
}

bool Renderer::loadTexture(const QString& path)
{
    if (path.isEmpty()) {
        qDebug() << "Renderer loadTexture failed: empty path";
        return false;
    }

    m_texturePath = toLocalFilePath(path);

    qDebug() << "Renderer stored texture path:" << m_texturePath;

    return true;
}

void Renderer::setZoom(float zoom)
{
    m_zoom = qMax(0.2f, qMin(5.0f, zoom));
    qDebug() << "Renderer zoom:" << m_zoom;
}

void Renderer::setRotation(float x, float y)
{
    // Convention:
    // +X slider means upward/orbit-style pitch.
    // +Y slider means right/orbit-style yaw.
    // Actual sign correction is applied in OpenGLViewport.cpp model matrix.
    m_rotationX = x;
    m_rotationY = y;

    qDebug() << "Renderer rotation:" << m_rotationX << m_rotationY;
}

void Renderer::resetCamera()
{
    m_zoom = 1.0f;
    m_rotationX = 0.0f;
    m_rotationY = 0.0f;

    qDebug() << "Renderer camera reset";
}

QString Renderer::modelPath() const
{
    return m_modelPath;
}

QString Renderer::texturePath() const
{
    return m_texturePath;
}

float Renderer::zoom() const
{
    return m_zoom;
}

float Renderer::rotationX() const
{
    return m_rotationX;
}

float Renderer::rotationY() const
{
    return m_rotationY;
}

int Renderer::vertexCount() const
{
    return static_cast<int>(m_loader.vertices.size());
}

int Renderer::uvCount() const
{
    return static_cast<int>(m_loader.uvs.size());
}

int Renderer::normalCount() const
{
    return static_cast<int>(m_loader.normals.size());
}