#include "texture.h"

#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QString>

Texture::~Texture()
{
    // IMPORTANT:
    // Do not call glDeleteTextures here.
    // The destructor can run when there is no current OpenGL context
    // or from a different thread than the render thread, which may crash.
    m_textureID = 0;
}

GLuint Texture::loadBMP_custom(const char* imagepath)
{
    if (imagepath == nullptr || imagepath[0] == '\0') {
        qDebug() << "Texture path is empty.";
        return 0;
    }

    const QString path = QString::fromUtf8(imagepath);
    qDebug() << "Reading texture image:" << path;

    QFileInfo info(path);
    qDebug() << "Texture exists:" << info.exists();
    qDebug() << "Texture is file:" << info.isFile();
    qDebug() << "Texture absolute path:" << info.absoluteFilePath();
    qDebug() << "Texture suffix:" << info.suffix();

    qDebug() << "Supported image formats:" << QImageReader::supportedImageFormats();

    QImageReader reader(path);
    qDebug() << "Texture reader canRead before read:" << reader.canRead();

    QImage image = reader.read();
    if (image.isNull()) {
        qDebug() << "Texture image could not be loaded:" << path;
        qDebug() << "QImageReader error:" << reader.errorString();
        return 0;
    }

    qDebug() << "Texture image loaded by QImageReader. Size:" << image.width() << "x" << image.height();

    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        qDebug() << "No current OpenGL context while loading texture.";
        return 0;
    }

    QOpenGLFunctions* f = context->functions();
    if (!f) {
        qDebug() << "Could not get QOpenGLFunctions.";
        return 0;
    }

    f->initializeOpenGLFunctions();

    // Keep image top-left based. loader.cpp already flips OBJ V with: uv.y = 1.0f - uv.y.
    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888);

    if (glImage.width() <= 0 || glImage.height() <= 0) {
        qDebug() << "Texture image has invalid size:" << glImage.size();
        return 0;
    }

    if (m_textureID != 0) {
        qDebug() << "Deleting previous texture ID:" << m_textureID;
        f->glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }

    f->glGenTextures(1, &m_textureID);
    if (m_textureID == 0) {
        qDebug() << "glGenTextures failed.";
        return 0;
    }

    qDebug() << "Generated texture ID:" << m_textureID;

    f->glBindTexture(GL_TEXTURE_2D, m_textureID);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Use no-mipmap filtering for the safe build.
    // This avoids crashes or GL errors in drivers/context setups where glGenerateMipmap is problematic.
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    qDebug() << "Uploading texture to GPU:" << glImage.width() << "x" << glImage.height();

    f->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        glImage.width(),
        glImage.height(),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        glImage.constBits()
        );

    GLenum error = f->glGetError();
    if (error != GL_NO_ERROR) {
        qDebug() << "glTexImage2D failed. OpenGL error:" << error;
        f->glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
        f->glBindTexture(GL_TEXTURE_2D, 0);
        return 0;
    }

    // Disabled intentionally for now:
    // f->glGenerateMipmap(GL_TEXTURE_2D);

    f->glBindTexture(GL_TEXTURE_2D, 0);

    qDebug() << "Texture loaded successfully. ID:" << m_textureID
             << "size:" << glImage.width() << "x" << glImage.height();

    return m_textureID;
}

GLuint Texture::loadDDS(const char* imagepath)
{
    Q_UNUSED(imagepath);

    qDebug() << "DDS loading is disabled in this safe build. Please use PNG/JPG/BMP.";
    return 0;
}

GLuint Texture::textureID() const
{
    return m_textureID;
}
