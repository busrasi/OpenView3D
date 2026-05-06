#ifndef OPENGL_WINDOW_H
#define OPENGL_WINDOW_H

#include "loader.h"
#include "shader.h"
#include "texture.h"
#include "controls.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>

class OpenGLWindow {
    public:
        /**
         * @brief C-tor
         */
        OpenGLWindow();

        /**
         * @brief D-tor
         */
        ~OpenGLWindow();

        void initialize(const int width, const int height);
        void initializeModelProperties();
        void SetWindowSize(const int width, const int height);
        void SetWindowPosition(const int x, const int y);
        void SetFullScreen(const bool isFullScreen);
        void SetBackgroundColor(const float R, const float G,const float B, const float alpha);
        void Render();

    private:
        GLFWwindow* m_window = nullptr;
        GLFWmonitor* m_monitor = nullptr;
        std::array<int, 2> m_windowSize{0, 0};
        std::array<int, 2> m_windowPosition{0, 0};
        std::array<float, 4> m_backgroundColor{0.f, 0.f, 0.f, 0.f};
        bool m_isFullScreen = false;

        Loader m_loader;
        Shader m_shader;
        Texture m_texture;
        Controls* m_controls;

        GLuint m_matrixID = 0;
        GLuint m_textureID = 0;
        GLuint m_lightID = 0;
};

#endif // OPENGL_WINDOW_H
