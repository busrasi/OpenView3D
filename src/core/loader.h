#ifndef LOADER_H
#define LOADER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

/**
 * @brief The Loader class
 */
class Loader {
    public:
        /**
        * @brief C-tor
        */
        Loader();

        /**
        * @brief D-tor
         */
        ~Loader();

        bool loadOBJ(const char* path);

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;

        glm::vec3 minimum{0}, maximum{0};
        GLuint vertexbuffer;
        GLuint uvbuffer;
};

#endif
