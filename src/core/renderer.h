#ifndef RENDERER_H
#define RENDERER_H

#include "loader.h"
#include "texture.h"

#include <QString>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool loadModel(const QString& path);
    bool loadTexture(const QString& path);

    void setZoom(float zoom);
    void setRotation(float x, float y);
    void resetCamera();

    QString modelPath() const;
    QString texturePath() const;

    float zoom() const;
    float rotationX() const;
    float rotationY() const;

    int vertexCount() const;
    int uvCount() const;
    int normalCount() const;

private:
    Loader m_loader;
    Texture m_texture;

    QString m_modelPath;
    QString m_texturePath;

    float m_zoom = 1.0f;
    float m_rotationX = 0.0f;
    float m_rotationY = 0.0f;
};

#endif