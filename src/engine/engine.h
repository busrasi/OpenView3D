#include <iostream>
#include <GLFW/glfw3.h>

#define WIDTH 640
#define HEIGHT 480
int main(int argc, char *argv[])
{
    GLFWwindow* window;
    int width, height;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Problem to initialize GLFW" << std::endl;
        exit(1);
    }
    // Create Window
    window = glfwCreateWindow(WIDTH, HEIGHT, "ARSUITE-ASSESTMENT", glfwGetPrimaryMonitor(), NULL);

    if (!window) {
        std::cout << "Problem to create GLFW window" << std::endl;
        // Terminate GLFW Object
        glfwTerminate();
        exit(1);
    }
    // Create OPENGL Context for Current Window
    glfwMakeContextCurrent(window);

    while(glfwWindowShouldClose(window) == 0 && glfwGetKey(window, GLFW_KEY_ESCAPE) == 0) {
        // Backgroud Color
     //   glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClearColor(184.0f/255.0f, 213.0f/255.0f, 238.0f/255.0f, 1.0f);

        // Buffers currently enabled for color writing
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Retrieves the size, in pixels, of the framebuffer
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glBegin(GL_TRIANGLES);
        glColor3f(1.f, 0.f, 0.f);
        glVertex3f(-0.6f, -0.4f, 0.f);
        glColor3f(0.f, 1.f, 0.f);
        glVertex3f(0.6f, -0.4f, 0.f);
        glColor3f(0.f, 0.f, 1.f);
        glVertex3f(0.f, 0.6f, 0.f);
        glEnd();

        // Swaps the front and back buffers of the specified window
        glfwSwapBuffers(window);
        // Processes all pending events
        glfwPollEvents();

    }

    glfwDestroyWindow(window);
    glfwTerminate();





    return 0;
}
