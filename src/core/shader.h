#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

/**
 * @brief The Shader class
 */
class Shader {
    public:
        /**
        * @brief C-tor
        */
        Shader() = default;

        /**
        * @brief D-tor
        */
        ~Shader();

        /**
        * @brief LoadShaders
        * @param vertex_file_path
        * @param fragment_file_path
        * @return
        */
        GLuint LoadShaders(const char * vertex_file_path,
                           const char * fragment_file_path);

        /**
        * @brief programID
        * @return
        */
        GLuint programID() const;

    private:
        GLuint m_programID = 0;
};

#endif // SHADER_H
