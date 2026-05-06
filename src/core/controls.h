#ifndef CONTROLS_H
#define CONTROLS_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

/**
 * @brief The Controls class
 */
class Controls {
    public:
        /**
        * @brief C-tor
        */
        Controls(GLFWwindow* window);

        /**
        * @brief D-tor
        */
        ~Controls();

        /**
        * @brief computeMatricesFromInputs
        */
        void computeMatricesFromInputs();

        /**
        * @brief getViewMatrix
        * @return
        */
        glm::mat4 getViewMatrix();

        /**
        * @brief getProjectionMatrix
        * @return
        */
        glm::mat4 getProjectionMatrix();

    private:
        GLFWwindow* m_window = nullptr;

        glm::mat4 m_viewMatrix;
        glm::mat4 m_projectionMatrix;

        // Initial position : on +Z
        glm::vec3 m_position = glm::vec3( 0, 0, 5 );
        // Initial horizontal angle : toward -Z
        float m_horizontalAngle = 3.14f;
        // Initial vertical angle : none
        float m_verticalAngle = 0.0f;
        // Initial Field of View
        float m_initialFoV = 45.0f;
        float m_speed = 3.0f; // 3 units / second
        float m_mouseSpeed = 0.005f;
};

#endif // CONTROLS_H
