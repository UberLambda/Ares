#include "Shader.hh"

#include <string>

namespace Ares
{
namespace GL33
{

ErrString checkShaderCompileError(GLuint shader)
{
    GLint ok = false;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if(!ok)
    {
        GLint logSize = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
        if(logSize == 0)
        {
            return "Unknown error (does the shader exist?)";
        }

        // Fetch the error
        // Note that string of size `logSize - 1` will hold `logSize` characters,
        // of which the last one will be a '\0' that can be written over by `glGetShaderInfoLog()`
        std::string err(logSize - 1, '\0');
        glGetShaderInfoLog(shader, logSize, nullptr, &err[0]);

        return ErrString(std::move(err));
    }
    else
    {
        // No error
        return {};
    }
}

ErrString compileShader(GLuint& outShader, GLenum type, const char* source)
{
    outShader = glCreateShader(type);
    if(!outShader)
    {
        return "Failed to create shader";
    }

    glShaderSource(outShader, 1, &source, nullptr);
    glCompileShader(outShader);

    auto err = checkShaderCompileError(outShader);
    if(err)
    {
        glDeleteShader(outShader); outShader = 0;
    }

    return std::move(err);
}


ErrString checkShaderProgramLinkError(GLuint shaderProgram)
{
    GLint ok = false;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
    if(!ok)
    {
        GLint logSize = 0;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logSize);
        if(logSize == 0)
        {
            return "Unknown error (does the shader program exist?)";
        }

        // Fetch the error
        // Note that string of size `logSize - 1` will hold `logSize` characters,
        // of which the last one will be a '\0' that can be written over by `glGetProgramInfoLog()`
        std::string err(logSize - 1, '\0');
        glGetProgramInfoLog(shaderProgram, logSize, nullptr, &err[0]);

        return ErrString(std::move(err));
    }
    else
    {
        // No error
        return {};
    }
}

}
}
