#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

/**
 * @brief The Texture class
 */
class Texture {
    public:
        /**
        * @brief C-tor
        */
        Texture() = default;

        /**
        * @brief D-tor
        */
        ~Texture();

        /**
        * @brief loadBMP_custom: Load a .BMP file using our custom loader
        * @param imagepath
        * @return
        */
        GLuint loadBMP_custom(const char * imagepath);

        /**
        * @brief loadDDS: Load a .DDS file using GLFW's own loader
        * @param imagepath
        * @return
        */
        GLuint loadDDS(const char * imagepath);

        /**
        * @brief texture
        * @return
        */
        GLuint textureID() const;

    private:
        GLuint m_textureID = 0;
};

#endif // TEXTURE_H
