#include "OpenGLViewport.h"

#include "core/loader.h"
#include "core/texture.h"

#include <QDebug>
#include <QQuickWindow>
#include <QMatrix4x4>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QUrl>

#include <cfloat>
#include <glm/glm.hpp>

class ViewportRenderer final
    : public QQuickFramebufferObject::Renderer,
      protected QOpenGLFunctions_3_3_Core
{
public:
    ViewportRenderer() = default;

    ~ViewportRenderer() override
    {
        if (m_normalbo != 0) {
            glDeleteBuffers(1, &m_normalbo);
            m_normalbo = 0;
        }

        if (m_uvbo != 0) {
            glDeleteBuffers(1, &m_uvbo);
            m_uvbo = 0;
        }

        if (m_vbo != 0) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }

        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
    }
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::Depth);
        format.setSamples(4);

        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject* item) override
    {
        auto* viewport = static_cast<OpenGLViewport*>(item);

        m_zoom = viewport->zoom();
        m_rotationX = viewport->rotationX();
        m_rotationY = viewport->rotationY();

        if (m_modelPath != viewport->modelPath()) {
            m_modelPath = viewport->modelPath();

            qDebug() << "ViewportRenderer received model path:" << m_modelPath;

            if (m_modelPath.isEmpty()) {
                m_modelLoaded = false;
                m_initialized = false;
            } else {
                QString localPath = m_modelPath;

                if (localPath.startsWith("file:///")) {
                    localPath = QUrl(m_modelPath).toLocalFile();
                }

                m_loader.vertices.clear();
                m_loader.uvs.clear();
                m_loader.normals.clear();

                m_modelLoaded = m_loader.loadOBJ(localPath.toUtf8().constData());

                qDebug() << "ViewportRenderer model loaded:" << m_modelLoaded;
                qDebug() << "Vertex count:" << m_loader.vertices.size();
                qDebug() << "UV count:" << m_loader.uvs.size();

                m_initialized = false;
            }
        }

        if (m_texturePath != viewport->texturePath()) {
            m_texturePath = viewport->texturePath();

            qDebug() << "ViewportRenderer received texture path:" << m_texturePath;

            m_textureLoaded = false;
            m_textureReloadPending = true;
        }
    }
    void render() override
    {
        initializeOpenGLFunctions();

        if (m_textureReloadPending) {
            m_textureReloadPending = false;

            if (!m_texturePath.isEmpty()) {
                QString localTexturePath = m_texturePath;

                if (localTexturePath.startsWith("file:///")) {
                    localTexturePath = QUrl(m_texturePath).toLocalFile();
                }
                GLuint texID = 0;

                if (localTexturePath.endsWith(".bmp", Qt::CaseInsensitive)) {
                    texID = m_texture.loadBMP_custom(localTexturePath.toUtf8().constData());
                } else if (localTexturePath.endsWith(".dds", Qt::CaseInsensitive)) {
                    qDebug() << "DDS loading disabled temporarily. Test with BMP first:" << localTexturePath;
                    texID = 0;
                } else {
                    qDebug() << "Unsupported texture format:" << localTexturePath;
                }

                m_textureLoaded = texID != 0;

                qDebug() << "ViewportRenderer texture loaded:" << m_textureLoaded;
                qDebug() << "Texture ID:" << texID;
                // GLuint texID = 0;

                // if (localTexturePath.endsWith(".dds", Qt::CaseInsensitive)) {
                //     texID = m_texture.loadDDS(localTexturePath.toUtf8().constData());
                // } else if (localTexturePath.endsWith(".bmp", Qt::CaseInsensitive)) {
                //     texID = m_texture.loadBMP_custom(localTexturePath.toUtf8().constData());
                // } else {
                //     qDebug() << "Unsupported texture format:" << localTexturePath;
                // }

                // m_textureLoaded = texID != 0;

                // qDebug() << "ViewportRenderer texture loaded:" << m_textureLoaded;
                // qDebug() << "Texture ID:" << texID;
            }
        }

        const int width = framebufferObject()->width();
        const int height = framebufferObject()->height();

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);

        glClearColor(0.956f, 0.956f, 0.956f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!m_modelLoaded) {
            update();
            return;
        }

        if (!m_initialized) {
            initializeModelBuffers();
        }

        if (!m_initialized) {
            update();
            return;
        }

        m_program.bind();
        if (m_textureLoaded && m_texture.textureID() != 0 && m_uvbo != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_texture.textureID());

            m_program.setUniformValue("myTextureSampler", 0);
            m_program.setUniformValue("useTexture", true);
        } else {
            m_program.setUniformValue("useTexture", false);
        }

        QMatrix4x4 projection;
        const float aspect = height > 0
                                 ? static_cast<float>(width) / static_cast<float>(height)
                                 : 1.0f;

        projection.perspective(45.0f, aspect, 0.1f, 100.0f);

        QMatrix4x4 view;
        view.translate(0.0f, 0.0f, -3.0f * m_zoom);

        QMatrix4x4 model;
        model.translate(-m_center.x, -m_center.y, -m_center.z);
        model.scale(m_scale * 2.0f);

        model.rotate(m_rotationX, 1.0f, 0.0f, 0.0f);
        model.rotate(m_rotationY, 0.0f, 1.0f, 0.0f);

        const QMatrix4x4 mvp = projection * view * model;
        m_program.setUniformValue("MVP", mvp);

        // if (m_textureLoaded && m_texture.textureID() != 0 && m_uvbo != 0) {
        //     glActiveTexture(GL_TEXTURE0);
        //     glBindTexture(GL_TEXTURE_2D, m_texture.textureID());

        //     m_program.setUniformValue("myTextureSampler", 0);
        //     m_program.setUniformValue("useTexture", true);
        // } else {
        //     m_program.setUniformValue("useTexture", false);
        // }

        glBindVertexArray(m_vao);
        glDrawArrays(
            GL_TRIANGLES,
            0,
            static_cast<GLsizei>(m_loader.vertices.size())
            );
        glBindVertexArray(0);

        m_program.release();

        update();
    }

private:
    void initializeModelBuffers()
    {
        if (m_loader.vertices.empty()) {
            qDebug() << "No vertices available for rendering.";
            return;
        }

        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);

        for (const auto& vertex : m_loader.vertices) {
            minBounds = glm::min(minBounds, vertex);
            maxBounds = glm::max(maxBounds, vertex);
        }

        m_center = (minBounds + maxBounds) * 0.5f;

        const glm::vec3 size = maxBounds - minBounds;
        const float largestDimension = glm::max(size.x, glm::max(size.y, size.z));

        if (largestDimension > 0.0f) {
            m_scale = 2.0f / largestDimension;
        } else {
            m_scale = 1.0f;
        }

        qDebug() << "Model center:" << m_center.x << m_center.y << m_center.z;
        qDebug() << "Model scale:" << m_scale;

        if (!m_program.isLinked()) {
            const bool vertexOk = m_program.addShaderFromSourceCode(
                QOpenGLShader::Vertex,
                R"(
                    #version 330 core

                    layout(location = 0) in vec3 position;
                    layout(location = 1) in vec2 vertexUV;

                    uniform mat4 MVP;

                    out vec2 UV;

                    void main()
                    {
                        gl_Position = MVP * vec4(position, 1.0);
                        UV = vertexUV;
                    }
                )"
                );

            if (!vertexOk) {
                qDebug() << "Vertex shader error:" << m_program.log();
                return;
            }

            const bool fragmentOk = m_program.addShaderFromSourceCode(
                QOpenGLShader::Fragment,
                R"(
                    #version 330 core

                    in vec2 UV;
                    out vec4 fragColor;

                    uniform sampler2D myTextureSampler;
                    uniform bool useTexture;

                    void main()
                    {
                        if (useTexture) {
                            fragColor = texture(myTextureSampler, UV);
                        } else {
                            fragColor = vec4(0.22, 0.24, 0.28, 1.0);
                        }
                    }
                )"
                );

            if (!fragmentOk) {
                qDebug() << "Fragment shader error:" << m_program.log();
                return;
            }

            if (!m_program.link()) {
                qDebug() << "Shader link error:" << m_program.log();
                return;
            }
        }

        if (m_uvbo != 0) {
            glDeleteBuffers(1, &m_uvbo);
            m_uvbo = 0;
        }

        if (m_vbo != 0) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }

        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_loader.vertices.size() * sizeof(glm::vec3)),
            m_loader.vertices.data(),
            GL_STATIC_DRAW
            );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(glm::vec3),
            reinterpret_cast<void*>(0)
            );

        if (!m_loader.uvs.empty()) {
            glGenBuffers(1, &m_uvbo);
            glBindBuffer(GL_ARRAY_BUFFER, m_uvbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(m_loader.uvs.size() * sizeof(glm::vec2)),
                m_loader.uvs.data(),
                GL_STATIC_DRAW
                );

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(
                1,
                2,
                GL_FLOAT,
                GL_FALSE,
                sizeof(glm::vec2),
                reinterpret_cast<void*>(0)
                );
        }

        glBindVertexArray(0);

        m_initialized = true;

        qDebug() << "Model buffers initialized.";
    }

private:
    bool m_initialized = false;
    bool m_modelLoaded = false;
    bool m_textureLoaded = false;
    bool m_textureReloadPending = false;

    QString m_modelPath;
    QString m_texturePath;

    float m_zoom = 1.0f;
    float m_rotationX = 0.0f;
    float m_rotationY = 0.0f;

    glm::vec3 m_center{0.0f, 0.0f, 0.0f};
    float m_scale = 1.0f;

    Loader m_loader;
    Texture m_texture;

    QOpenGLShaderProgram m_program;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_uvbo = 0;
    GLuint m_normalbo = 0;
};

OpenGLViewport::OpenGLViewport(QQuickItem* parent)
    : QQuickFramebufferObject(parent)
{
}

QString OpenGLViewport::modelPath() const
{
    return m_modelPath;
}

void OpenGLViewport::setModelPath(const QString& path)
{
    if (m_modelPath == path) {
        return;
    }

    m_modelPath = path;
    emit modelPathChanged();

    update();
}

QString OpenGLViewport::texturePath() const
{
    return m_texturePath;
}

void OpenGLViewport::setTexturePath(const QString& path)
{
    if (m_texturePath == path) {
        return;
    }

    m_texturePath = path;

    qDebug() << "OpenGLViewport texturePath set:" << m_texturePath;

    emit texturePathChanged();

    update();

    if (window()) {
        window()->update();
    }
}

float OpenGLViewport::zoom() const
{
    return m_zoom;
}

void OpenGLViewport::setZoom(float value)
{
    if (qFuzzyCompare(m_zoom, value)) {
        return;
    }

    m_zoom = value;
    emit zoomChanged();

    update();
}

float OpenGLViewport::rotationX() const
{
    return m_rotationX;
}

void OpenGLViewport::setRotationX(float value)
{
    if (qFuzzyCompare(m_rotationX, value)) {
        return;
    }

    m_rotationX = value;
    emit rotationXChanged();

    update();
}

float OpenGLViewport::rotationY() const
{
    return m_rotationY;
}

void OpenGLViewport::setRotationY(float value)
{
    if (qFuzzyCompare(m_rotationY, value)) {
        return;
    }

    m_rotationY = value;
    emit rotationYChanged();

    update();
}

QQuickFramebufferObject::Renderer* OpenGLViewport::createRenderer() const
{
    return new ViewportRenderer();
}