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
#include <QVector3D>

#include <cfloat>
#include <glm/glm.hpp>

#include <vector>
#include <QPointer>
#include <cmath>
#include <algorithm>
#include <map>
#include <QQuickOpenGLUtils>
#include <QScopeGuard>

class ViewportRenderer final
    : public QQuickFramebufferObject::Renderer,
      protected QOpenGLFunctions_3_3_Core
{
public:
    ViewportRenderer() = default;

    ~ViewportRenderer() override
    {
        const GLuint texture = m_texture.textureID();
        if (texture) glDeleteTextures(1, &texture);
        for (const auto& entry : m_assetBuffers) {
            glDeleteBuffers(1, &entry.second.vbo);
            glDeleteVertexArrays(1, &entry.second.vao);
        }
        if (m_gridVbo != 0) {
            glDeleteBuffers(1, &m_gridVbo);
            m_gridVbo = 0;
        }

        if (m_gridVao != 0) {
            glDeleteVertexArrays(1, &m_gridVao);
            m_gridVao = 0;
        }

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
        m_panX = viewport->panX();
        m_panY = viewport->panY();

        m_viewport = viewport;
        const auto mesh = viewport->meshData().value<MeshPtr>();
        if (m_mesh != mesh) {
            m_mesh = mesh;
            m_modelLoaded = bool(mesh);
            m_initialized = false;
            m_errorReported = false;
        }
        m_scene = viewport->sceneData().value<ScenePtr>();
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
        const auto restoreQtState = qScopeGuard([] { QQuickOpenGLUtils::resetOpenGLState(); });
        if (!m_scene && !m_assetBuffers.empty()) {
            for (const auto& entry : m_assetBuffers) {
                glDeleteBuffers(1,&entry.second.vbo); glDeleteVertexArrays(1,&entry.second.vao);
            }
            m_assetBuffers.clear();
        }

        if (m_textureReloadPending) {
            m_textureReloadPending = false;
            loadTextureIfNeeded();
        }

        const int width = framebufferObject()->width();
        const int height = framebufferObject()->height();

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);

        glClearColor(0.956f, 0.956f, 0.956f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!m_modelLoaded) {
            return;
        }

        if (!m_initialized) {
            initializeModelBuffers();
        }

        if (!m_initialized) {
            reportError("Could not create model rendering resources.");
            return;
        }

        if (m_scene) { renderScene(width,height); return; }
        QMatrix4x4 projection;
        const float aspect = height > 0
                                 ? static_cast<float>(width) / static_cast<float>(height)
                                 : 1.0f;

        projection.perspective(45.0f, aspect, 0.1f, 100.0f);

        QMatrix4x4 view;
        view.lookAt(
            QVector3D(0.0f, 0.0f, 3.0f * m_zoom),
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(0.0f, 1.0f, 0.0f)
            );

        // Viewer coordinate convention fix:
        // +X = screen right / EAST
        // +Y = screen up / NORTH
        // +Z = depth/front direction
        QMatrix4x4 worldConvention;
        worldConvention.scale(1.0f, -1.0f, 1.0f);

        // Ground grid:
        // Drawn in the XZ plane below the model using the same viewer convention.
        renderGroundGrid(projection, view, worldConvention);

        QMatrix4x4 model = worldConvention;

        // Final control convention:
        // X Rotation increase -> model tilts upward/forward.
        // Y Rotation increase -> model turns clockwise/right in the viewport.
        model.rotate(-m_rotationX, 1.0f, 0.0f, 0.0f);
        model.rotate( m_rotationY, 0.0f, 1.0f, 0.0f);
        // Qt post-multiplies: center the source geometry first, then scale/rotate.
        // Translating before scaling leaves off-origin OBJ models outside the view.
        model.scale(m_scale * 2.0f);
        model.translate(-m_center.x, -m_center.y, -m_center.z);

        // Mouse drag / object pan:
        // QML mouse Y increases downward, but viewer +Y is upward.
        // Therefore panY is inverted here.
        QMatrix4x4 panMatrix;
        panMatrix.translate(m_panX, -m_panY, 0.0f);
        model = panMatrix * model;

        const QVector3D cameraPosition(0.0f, 0.0f, 3.0f * m_zoom);

        renderModel(projection, view, model, cameraPosition);
    }

private:
    void reportError(const QString& error) {
        if (m_errorReported || !m_viewport) return;
        m_errorReported = true;
        QMetaObject::invokeMethod(m_viewport.data(), [item=m_viewport,error] {
            if (item) emit item->renderingFailed(error);
        }, Qt::QueuedConnection);
    }
    bool initializeAssets() {
        // Retain immutable assets shared by consecutive scenes; release unused GPU buffers.
        for (auto it=m_assetBuffers.begin();it!=m_assetBuffers.end();) {
            if (!m_scene->assets.contains(it->first)) {
                glDeleteBuffers(1,&it->second.vbo); glDeleteVertexArrays(1,&it->second.vao);
                it=m_assetBuffers.erase(it);
            } else ++it;
        }
        for (auto it=m_scene->assets.begin();it!=m_scene->assets.end();++it) {
            if (m_assetBuffers.count(it.key())) continue;
            const auto& vertices=it.value()->vertices;
            AssetBuffers buffers;
            glGenVertexArrays(1,&buffers.vao); glBindVertexArray(buffers.vao);
            glGenBuffers(1,&buffers.vbo); glBindBuffer(GL_ARRAY_BUFFER,buffers.vbo);
            glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(vertices.size()*sizeof(float)),vertices.data(),GL_STATIC_DRAW);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);
            glEnableVertexAttribArray(2); glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
            glBindVertexArray(0);
            if (glGetError()!=GL_NO_ERROR || !buffers.vao || !buffers.vbo) {
                if (buffers.vao) glDeleteVertexArrays(1,&buffers.vao);
                if (buffers.vbo) glDeleteBuffers(1,&buffers.vbo);
                reportError("Could not allocate environment geometry on the GPU."); return false;
            }
            buffers.count=GLsizei(vertices.size()/6);
            m_assetBuffers.emplace(it.key(),buffers);
        }
        return true;
    }    void renderScene(int width, int height) {
        if (!initializeAssets()) return;
        const float aspect = float(width)/std::max(1,height);
        const float halfFov = std::min(.392699f,std::atan(std::tan(.392699f)*aspect));
        const float distance = m_scene->cameraRadius / std::sin(halfFov) * std::max(.1f,m_zoom);
        const float yaw = (.35f + m_rotationY*.0174533f);
        const float pitch = std::clamp(.24f + m_rotationX*.0174533f,-1.45f,1.45f);
        const auto target = m_scene->cameraTarget + QVector3D(m_panX,-m_panY,0);
        const auto camera = target + QVector3D(std::sin(yaw)*std::cos(pitch),std::sin(pitch),std::cos(yaw)*std::cos(pitch))*distance;
        QMatrix4x4 view, projection;
        view.lookAt(camera,target,{0,1,0});
        projection.perspective(45,aspect,.01f,std::max(100.f,distance+(m_scene->maximum-m_scene->minimum).length()*2));
        // Qt Quick presents this FBO vertically inverted. Correct presentation in
        // clip space, leaving the room, hero, camera and lighting consistently Y-up.
        projection(1,1) = -projection(1,1);
        glDisable(GL_BLEND); glDisable(GL_CULL_FACE);
        renderModel(projection,view,m_scene->heroTransform,camera);
        m_program.bind();
        m_program.setUniformValue("useTexture",false); m_program.setUniformValue("useNormals",true);

        for (const auto& object : m_scene->objects) {
            // Cut away the wall facing an outside camera, so orbiting never hides the hero.
            if (object.id.endsWith("/left_wall") && camera.x() < -m_scene->spec.width/2) continue;
            if (object.id.endsWith("/back_wall") && camera.z() < -m_scene->spec.depth/2) continue;
            m_program.setUniformValue("MVP",projection*view*object.transform);
            m_program.setUniformValue("ModelMatrix",object.transform);
            m_program.setUniformValue("materialColor",object.material.color);
            m_program.setUniformValue("specularStrength",object.material.specular);
            m_program.setUniformValue("shininess",object.material.shininess);
            const auto buffer = m_assetBuffers.find(object.asset);
            if (buffer == m_assetBuffers.end()) { reportError("Scene references a missing asset."); continue; }
            glBindVertexArray(buffer->second.vao);
            glDrawArrays(GL_TRIANGLES,0,buffer->second.count);
        }
        glBindVertexArray(0); m_program.release();
        if (glGetError() != GL_NO_ERROR) reportError("OpenGL reported an error while rendering the generated scene.");
    }
    void loadTextureIfNeeded()
    {
        m_textureLoaded = false;

        if (m_texturePath.isEmpty()) {
            return;
        }

        QString localTexturePath = m_texturePath;

        if (localTexturePath.startsWith("file://")) {
            localTexturePath = QUrl(m_texturePath).toLocalFile();
        }

        GLuint texID = 0;

        if (localTexturePath.endsWith(".dds", Qt::CaseInsensitive)) {
            qDebug() << "DDS loading disabled in safe mode. Convert this file to PNG/JPG/BMP:" << localTexturePath;
            texID = 0;
        } else if (localTexturePath.endsWith(".bmp", Qt::CaseInsensitive)
                   || localTexturePath.endsWith(".png", Qt::CaseInsensitive)
                   || localTexturePath.endsWith(".jpg", Qt::CaseInsensitive)
                   || localTexturePath.endsWith(".jpeg", Qt::CaseInsensitive)) {
            texID = m_texture.loadBMP_custom(localTexturePath.toUtf8().constData());
        } else {
            qDebug() << "Unsupported texture format:" << localTexturePath;
        }

        m_textureLoaded = texID != 0;

        qDebug() << "ViewportRenderer texture local path:" << localTexturePath;
        qDebug() << "ViewportRenderer texture loaded:" << m_textureLoaded;
        qDebug() << "Texture ID:" << texID;
    }

    void renderModel(const QMatrix4x4& projection,
                     const QMatrix4x4& view,
                     const QMatrix4x4& model,
                     const QVector3D& cameraPosition)
    {
        m_program.bind();

        const bool canUseTexture = m_textureLoaded
                                   && m_texture.textureID() != 0
                                   && m_uvbo != 0
                                   && m_mesh->uvs.size() == m_mesh->vertices.size();

        if (canUseTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_texture.textureID());
            m_program.setUniformValue("myTextureSampler", 0);
            m_program.setUniformValue("useTexture", true);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            m_program.setUniformValue("useTexture", false);
        }

        const QMatrix4x4 mvp = projection * view * model;
        m_program.setUniformValue("MVP", mvp);
        m_program.setUniformValue("ModelMatrix", model);

        const bool canUseNormals = m_normalbo != 0
                                   && m_mesh->normals.size() == m_mesh->vertices.size();

        m_program.setUniformValue("useNormals", canUseNormals);

        // Fixed studio-style light: left, top, front.
        // In the finalized viewer convention:
        // X negative = left, Y positive = top, Z positive = camera/front.
        m_program.setUniformValue("lightDirection", (m_scene ? m_scene->light.direction : QVector3D(-.45f,.75f,.55f)).normalized());
        m_program.setUniformValue("cameraPosition", cameraPosition);

        m_program.setUniformValue("materialColor", m_scene ? QVector3D(.65f,.71f,.76f) : QVector3D(.22f,.24f,.28f));
        m_program.setUniformValue("lightColor", m_scene ? m_scene->light.color : QVector3D(1,1,1));
        m_program.setUniformValue("ambientStrength", m_scene ? m_scene->light.ambient : .4f);
        m_program.setUniformValue("diffuseStrength", m_scene ? m_scene->light.diffuse : .7f);
        m_program.setUniformValue("specularStrength", 0.15f);
        m_program.setUniformValue("shininess", 96.0f);

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_mesh->vertices.size()));
        glBindVertexArray(0);

        m_program.release();
    }

    void renderGroundGrid(const QMatrix4x4& projection,
                          const QMatrix4x4& view,
                          const QMatrix4x4& worldConvention)
    {
        initializeGroundGrid();

        if (!m_gridProgram.isLinked() || m_gridVao == 0) {
            return;
        }

        const QMatrix4x4 mvp = projection * view * worldConvention;

        glEnable(GL_DEPTH_TEST);

        // The grid fragment shader uses alpha. Enable blending so the grid
        // appears as a soft CAD helper instead of bright/white opaque lines.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTexture(GL_TEXTURE_2D, 0);

        m_gridProgram.bind();
        m_gridProgram.setUniformValue("MVP", mvp);
        m_gridProgram.setUniformValue(
            "gridColor",
            0.45f,
            0.48f,
            0.55f
            );

        glBindVertexArray(m_gridVao);
        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, m_gridVertexCount);
        glBindVertexArray(0);

        m_gridProgram.release();

        glDisable(GL_BLEND);
    }

    void initializeGroundGrid()
    {
        if (!m_gridProgram.isLinked()) {
            const bool vertexOk = m_gridProgram.addShaderFromSourceCode(
                QOpenGLShader::Vertex,
                R"(
                    #version 330 core

                    layout(location = 0) in vec3 position;

                    uniform mat4 MVP;

                    void main()
                    {
                        gl_Position = MVP * vec4(position, 1.0);
                    }
                )"
                );

            if (!vertexOk) {
                qDebug() << "Ground grid vertex shader error:" << m_gridProgram.log();
                return;
            }

            const bool fragmentOk = m_gridProgram.addShaderFromSourceCode(
                QOpenGLShader::Fragment,
                R"(
                    #version 330 core

                    out vec4 fragColor;

                    uniform vec3 gridColor;

                    void main()
                    {
                        fragColor = vec4(gridColor, 1.0);
                    }
                )"
                );

            if (!fragmentOk) {
                qDebug() << "Ground grid fragment shader error:" << m_gridProgram.log();
                return;
            }

            if (!m_gridProgram.link()) {
                qDebug() << "Ground grid shader link error:" << m_gridProgram.log();
                return;
            }
        }

        if (m_gridVao != 0) {
            return;
        }

        // The grid is built in model/world coordinates and then receives
        // worldConvention in renderGroundGrid(). Its Y value is computed from
        // the currently loaded model bounds so it sits below the object.
        constexpr int halfLineCount = 10;
        constexpr float spacing = 0.25f;
        const float gridY = m_gridY;
        constexpr float extent = halfLineCount * spacing;

        std::vector<float> vertices;
        vertices.reserve((halfLineCount * 2 + 1) * 12);

        for (int i = -halfLineCount; i <= halfLineCount; ++i) {
            const float v = static_cast<float>(i) * spacing;

            // Line parallel to X.
            vertices.push_back(-extent);
            vertices.push_back(gridY);
            vertices.push_back(v);

            vertices.push_back(extent);
            vertices.push_back(gridY);
            vertices.push_back(v);

            // Line parallel to Z.
            vertices.push_back(v);
            vertices.push_back(gridY);
            vertices.push_back(-extent);

            vertices.push_back(v);
            vertices.push_back(gridY);
            vertices.push_back(extent);
        }

        m_gridVertexCount = static_cast<GLsizei>(vertices.size() / 3);

        glGenVertexArrays(1, &m_gridVao);
        glBindVertexArray(m_gridVao);

        glGenBuffers(1, &m_gridVbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_gridVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(),
            GL_STATIC_DRAW
            );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            reinterpret_cast<void*>(0)
            );

        glBindVertexArray(0);

        qDebug() << "Ground grid initialized. Vertex count:" << m_gridVertexCount;
    }

    void initializeModelBuffers()
    {
        if (m_mesh->vertices.empty()) {
            qDebug() << "No vertices available for rendering.";
            return;
        }

        const auto minBounds = m_mesh->minimum;
        const auto maxBounds = m_mesh->maximum;
        m_center = (minBounds + maxBounds) * 0.5f;

        const glm::vec3 size = maxBounds - minBounds;
        const float largestDimension = glm::max(size.x, glm::max(size.y, size.z));

        if (largestDimension > 0.0f) {
            m_scale = 2.0f / largestDimension;
        } else {
            m_scale = 1.0f;
        }

        // Dynamic ground grid placement:
        // The model is centered and scaled before rendering, so compute the visual
        // half-height in the same scaled space. The grid is then placed just below
        // the model instead of using a fixed Y value that can cut through different models.
        const float scaledHalfHeight = (size.y * 0.5f) * (m_scale * 2.0f);
        m_gridY = -(scaledHalfHeight + 0.03f);

        // Rebuild grid buffer for the new model because grid Y depends on model bounds.
        if (m_gridVbo != 0) {
            glDeleteBuffers(1, &m_gridVbo);
            m_gridVbo = 0;
        }

        if (m_gridVao != 0) {
            glDeleteVertexArrays(1, &m_gridVao);
            m_gridVao = 0;
        }

        m_gridVertexCount = 0;

        qDebug() << "Model center:" << m_center.x << m_center.y << m_center.z;
        qDebug() << "Model scale:" << m_scale;
        qDebug() << "Dynamic grid Y:" << m_gridY;

        initializeModelShader();

        if (!m_program.isLinked()) {
            return;
        }

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

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_mesh->vertices.size() * sizeof(glm::vec3)),
            m_mesh->vertices.data(),
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

        if (!m_mesh->uvs.empty() && m_mesh->uvs.size() == m_mesh->vertices.size()) {
            glGenBuffers(1, &m_uvbo);
            glBindBuffer(GL_ARRAY_BUFFER, m_uvbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(m_mesh->uvs.size() * sizeof(glm::vec2)),
                m_mesh->uvs.data(),
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
        } else {
            qDebug() << "Texture disabled: missing UVs or UV/vertex count mismatch."
                     << "vertices:" << m_mesh->vertices.size()
                     << "uvs:" << m_mesh->uvs.size();
        }

        if (!m_mesh->normals.empty() && m_mesh->normals.size() == m_mesh->vertices.size()) {
            glGenBuffers(1, &m_normalbo);
            glBindBuffer(GL_ARRAY_BUFFER, m_normalbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(m_mesh->normals.size() * sizeof(glm::vec3)),
                m_mesh->normals.data(),
                GL_STATIC_DRAW
                );

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(
                2,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(glm::vec3),
                reinterpret_cast<void*>(0)
                );

            qDebug() << "Normal buffer initialized. Normal count:" << m_mesh->normals.size();
        } else {
            qDebug() << "Lighting fallback: missing normals or normal/vertex count mismatch."
                     << "vertices:" << m_mesh->vertices.size()
                     << "normals:" << m_mesh->normals.size();
        }

        glBindVertexArray(0);

        m_initialized = m_vao && m_vbo && glGetError() == GL_NO_ERROR;

        qDebug() << "Model buffers initialized.";
    }

    void initializeModelShader()
    {
        if (m_program.isLinked()) {
            return;
        }

        const bool vertexOk = m_program.addShaderFromSourceCode(
            QOpenGLShader::Vertex,
            R"(
                #version 330 core

                layout(location = 0) in vec3 position;
                layout(location = 1) in vec2 vertexUV;
                layout(location = 2) in vec3 vertexNormal;

                uniform mat4 MVP;
                uniform mat4 ModelMatrix;
                uniform bool useNormals;

                out vec2 UV;
                out vec3 WorldPosition;
                out vec3 WorldNormal;

                void main()
                {
                    vec4 worldPos = ModelMatrix * vec4(position, 1.0);

                    gl_Position = MVP * vec4(position, 1.0);
                    UV = vertexUV;
                    WorldPosition = worldPos.xyz;

                    if (useNormals) {
                        WorldNormal = normalize(mat3(transpose(inverse(ModelMatrix))) * vertexNormal);
                    } else {
                        WorldNormal = vec3(0.0, 0.0, 1.0);
                    }
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
                in vec3 WorldPosition;
                in vec3 WorldNormal;

                out vec4 fragColor;

                uniform sampler2D myTextureSampler;
                uniform bool useTexture;
                uniform bool useNormals;

                uniform vec3 lightDirection;
                uniform vec3 cameraPosition;

                uniform vec3 materialColor;
                uniform vec3 lightColor;
                uniform float ambientStrength;
                uniform float diffuseStrength;
                uniform float specularStrength;
                uniform float shininess;

                void main()
                {
                    vec3 baseColor = useTexture
                        ? texture(myTextureSampler, UV).rgb
                        : materialColor;

                    vec3 normal = normalize(WorldNormal);
                    vec3 lightDir = normalize(lightDirection);

                    float diffuseFactor = useNormals
                        ? max(dot(normal, lightDir), 0.0)
                        : 1.0;

                    vec3 ambient = baseColor * ambientStrength;
                    vec3 diffuse = baseColor * diffuseFactor * diffuseStrength;

                    vec3 viewDir = normalize(cameraPosition - WorldPosition);
                    vec3 halfDir = normalize(lightDir + viewDir);

                    float specularFactor = useNormals
                        ? pow(max(dot(normal, halfDir), 0.0), shininess)
                        : 0.0;

                    vec3 specular = vec3(1.0) * specularFactor * specularStrength;

                    vec3 finalColor = (ambient + diffuse + specular) * lightColor;

                    fragColor = vec4(finalColor, 1.0);
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
    float m_panX = 0.0f;
    float m_panY = 0.0f;

    glm::vec3 m_center{0.0f, 0.0f, 0.0f};
    float m_scale = 1.0f;
    float m_gridY = -1.05f;

    MeshPtr m_mesh;
    ScenePtr m_scene;
    QPointer<OpenGLViewport> m_viewport;
    bool m_errorReported = false;
    struct AssetBuffers { GLuint vao = 0, vbo = 0; GLsizei count = 0; };
    std::map<QString,AssetBuffers> m_assetBuffers;
    Texture m_texture;

    QOpenGLShaderProgram m_program;
    QOpenGLShaderProgram m_gridProgram;

    GLuint m_gridVao = 0;
    GLuint m_gridVbo = 0;
    GLsizei m_gridVertexCount = 0;

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
float OpenGLViewport::panX() const
{
    return m_panX;
}

void OpenGLViewport::setPanX(float value)
{
    if (qFuzzyCompare(m_panX, value)) {
        return;
    }

    m_panX = value;
    emit panXChanged();

    update();
}

float OpenGLViewport::panY() const
{
    return m_panY;
}

void OpenGLViewport::setPanY(float value)
{
    if (qFuzzyCompare(m_panY, value)) {
        return;
    }

    m_panY = value;
    emit panYChanged();

    update();
}
