#include "openGLWindow.h"
#include "config.h"

#include <iostream>
#include <vector>

#include <glm/glm.hpp>


OpenGLWindow::OpenGLWindow()
    : m_window(nullptr),
      m_monitor(nullptr),
      m_windowSize{640,480},
      m_windowPosition{0,0},
      m_isFullScreen(false)
{}

OpenGLWindow::~OpenGLWindow()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void OpenGLWindow::initialize(const int width, const int height)
{
    // Update window size
    SetWindowSize(width,height);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Problem to initialize GLFW" << std::endl;
        exit(1);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create Window - glfwGetPrimaryMonitor()-
    m_window = glfwCreateWindow(m_windowSize[0], m_windowSize[1], "ARSUITE-ASSESTMENT", NULL, NULL);

    if (!m_window) {
        std::cout << "Problem to create GLFW window" << std::endl;
        // Terminate GLFW Object
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(m_window);
    m_monitor = glfwGetPrimaryMonitor();

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return;
    }

    // Update controls
    m_controls = new Controls(m_window);

}

void OpenGLWindow::initializeModelProperties()
{
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    m_shader.LoadShaders(
                std::string(openview3d::resourcesPath + "/shaders/TransformVertexShader.vertexshader").c_str(),
                std::string(openview3d::resourcesPath + "/shaders/TextureFragmentShader.fragmentshader").c_str()
                );

    // Get a handle for our "MVP" uniform
    m_matrixID = glGetUniformLocation(m_shader.programID(), "MVP");

    // Load the texture
    m_texture.loadDDS(std::string(openview3d::resourcesPath + "/models/capsule0.DDS").c_str());

    // Get a handle for our "myTextureSampler" uniform
    m_textureID = glGetUniformLocation(m_shader.programID(), "myTextureSampler");

    // Light
    m_lightID = glGetUniformLocation(m_shader.programID(), "LightPosition_worldspace");

    // Read .obj file
    m_loader.loadOBJ(std::string(openview3d::resourcesPath + "/models/capsule.obj").c_str());

    // Load it into a VBO
    glGenBuffers(1, &m_loader.vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_loader.vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, m_loader.vertices.size() * sizeof(glm::vec3), &m_loader.vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &m_loader.uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_loader.uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, m_loader.uvs.size() * sizeof(glm::vec2), &m_loader.uvs[0], GL_STATIC_DRAW);
}

void OpenGLWindow::SetWindowSize(const int width, const int height)
{
    m_windowSize[0] = width;
    m_windowSize[1] = height;
}

void OpenGLWindow::SetWindowPosition(const int x, const int y)
{
    m_windowPosition[0] = x;
    m_windowPosition[1] = y;
}

void OpenGLWindow::SetFullScreen(const bool isFullScreen)
{
    m_isFullScreen = isFullScreen;
    if(m_isFullScreen == true){
        // Store window position
        glfwGetWindowPos(m_window, &m_windowPosition[0], &m_windowPosition[1]);
        // Store window size
        glfwGetWindowSize(m_window, &m_windowSize[0], &m_windowSize[1]);
        // Store resolution of monitor
        const GLFWvidmode * mode = glfwGetVideoMode(m_monitor);
        // Switch to fullscreen
        glfwSetWindowMonitor(m_window, m_monitor, 0, 0, mode->width, mode->height, 0);
    }
    else {
        // Switch to fullscreen
        glfwSetWindowMonitor(m_window, nullptr, m_windowPosition[0], m_windowPosition[1], m_windowSize[0], m_windowSize[1], 0);
    }
}

void OpenGLWindow::SetBackgroundColor(const float R, const float G, const float B, const float alpha)
{
    m_backgroundColor[0] = R;
    m_backgroundColor[1] = G;
    m_backgroundColor[2] = B;
    m_backgroundColor[3] = alpha;
}

void OpenGLWindow::Render()
{
    while(glfwWindowShouldClose(m_window) == 0 && glfwGetKey(m_window, GLFW_KEY_ESCAPE) == 0) {
        // Backgroud Color
        glClearColor(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], m_backgroundColor[3]);

        // Buffers currently enabled for color writing
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Retrieves the size, in pixels, of the framebuffer
        glfwGetFramebufferSize(m_window, &m_windowSize[0], &m_windowSize[1]);
        glViewport(m_windowPosition[0], m_windowPosition[1], m_windowSize[0], m_windowSize[1]);

        // Use our shader
        glUseProgram(m_shader.programID());

        // Compute the MVP matrix from keyboard and mouse input
        m_controls->computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = m_controls->getProjectionMatrix();
        glm::mat4 ViewMatrix = m_controls->getViewMatrix();
        glm::mat4 ModelMatrix = glm::mat4(1.0);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        // Light position
        glm::vec3 lightPos = glm::vec3(4, 4, 4);
        glUniform3f(m_lightID, lightPos.x, lightPos.y, lightPos.z);

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(m_matrixID, 1, GL_FALSE, &MVP[0][0]);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture.textureID());

        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(m_textureID, 0);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, m_loader.vertexbuffer);
        glVertexAttribPointer(
                    0,                  // attribute
                    3,                  // size
                    GL_FLOAT,           // type
                    GL_FALSE,           // normalized?
                    0,                  // stride
                    (void*)0            // array buffer offset
                    );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, m_loader.uvbuffer);
        glVertexAttribPointer(
                    1,                                // attribute
                    2,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
                    );

        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, m_loader.vertices.size() );

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);


        // Swaps the front and back buffers of the specified window
        glfwSwapBuffers(m_window);

        // Processes all pending events
        glfwPollEvents();
    }
}
