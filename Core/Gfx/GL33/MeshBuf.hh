#pragma once

#include <flextGL.h>
#include "../Resource/Mesh.hh"

namespace Ares
{
namespace GL33
{

/// Copies data to an OpenGL vertex and index buffer from an `Ares::Mesh`.
class MeshBuf
{
    GLuint vertexBuf_, indexBuf_;
    GLsizei nVertices_, nIndices_;

public:
    /// Initializes a new, empty `MeshBuf`.
    /// `nVertices()` and `nIndices()` will initially be zero.
    MeshBuf();

    /// Destroys the meshbuf and its internal buffers.
    ~MeshBuf();

    /// Returns `true` if the meshbuf is valid and not empty (i.e. has the two
    /// internal OpenGL buffers, atleast one vertex and atleast one index).
    operator bool() const;


    /// Uploads the given mesh to `vetexBuffer` and `indexBuffer`. Returns `false`
    /// on failure (usually because the internal buffers were not created
    /// successfully at startup time).
    /// Usage is a data usage hint for OpenGL to know what to do with the buffers.
    /// **OpenGL**: rebinds `GL_ARRAY_BUFFER`, `GL_ELEMENT_ARRAY_BUFFER`.
    bool upload(const Mesh& mesh, GLenum usage=GL_STATIC_DRAW);


    /// Returns the OpenGL name for the internal vertex buffer.
    /// Bind it to `GL_ARRAY_BUFFER` for rendering.
    inline GLuint vertexBuffer() const
    {
        return vertexBuf_;
    }

    /// Returns the number of vertices currently stored in the vertex buffer.
    /// Can return `0` for empty meshes.
    inline GLsizei nVertices() const
    {
        return nVertices_;
    }

    /// Returns the OpenGL name for the internal index buffer.
    /// Bind it to `GL_ELEMENT_ARRAY_BUFFER` for rendering.
    inline GLuint indexBuffer() const
    {
        return indexBuf_;
    }

    /// Returns the number of indices currently stored in the index buffer.
    /// Can return `0` for empty meshes.
    inline GLsizei nIndices() const
    {
        return nIndices_;
    }
};

}
}
