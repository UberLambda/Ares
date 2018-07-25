#pragma once

#include <flextGL.h>
#include <Core/Base/ErrString.hh>

namespace Ares
{
namespace GL33
{

/// Returns an error string containing a shader's info log if the `GL_COMPILE_STATUS`
/// of the shader is not 1.
ErrString checkShaderCompileError(GLuint shader);

/// Attempts to create and compile a GLSL 330 core shader of the given type from
/// source. Returns an error string if compilation failed.
///
/// Shaders whose compilation has failed are deleted before returning. Note that
/// `outShader` can be overwritten even on failure.
ErrString compileShader(GLuint& outShader, GLenum type, const char* source);


/// Returns an error string containing a shader program's info log if the
/// `GL_LINK_STATUS` of the shader program is not 1.
ErrString checkShaderProgramLinkError(GLuint shaderProgramrogram);

/// Attempts to create and link a GLSL 330 core shader program given the shaders
/// that are to be attached to it. Returns an error string if compilation failed.
///
/// All shaders are detached from the output program so they can be deleted after
/// this call succesfully returns.
///
/// Note that this may overwrite `outProgram` even on failure.
/// Shaders program whose linkage has failed are deleted before returning. Note
/// that `outProgram` can be overwritten even on failure.
template <typename ShaderIterator>
ErrString linkShaderProgram(GLuint& outProgram, ShaderIterator begin, ShaderIterator end)
{
    outProgram = glCreateProgram();
    if(!outProgram)
    {
        return "Failed to create shader program";
    }

    for(auto it = begin; it != end; it ++)
    {
        if(*it != 0)
        {
            glAttachShader(outProgram, *it);
        }
    }
    glLinkProgram(outProgram);
    for(auto it = begin; it != end; it ++)
    {
        if(*it != 0)
        {
            glDetachShader(outProgram, *it);
        }
    }

    auto err = checkShaderProgramLinkError(outProgram);
    if(err)
    {
        glpfDeleteProgram(outProgram); outProgram = 0;
    }

    return std::move(err);
}

}
}

