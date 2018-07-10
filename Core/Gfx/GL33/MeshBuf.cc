#include "MeshBuf.hh"

namespace Ares
{
namespace GL33
{

MeshBuf::MeshBuf()
    : vertexBuf_(0), nVertices_(0),
      indexBuf_(0), nIndices_(0)
{
    glGenBuffers(1, &vertexBuf_);
    glGenBuffers(1, &indexBuf_);
}

MeshBuf::~MeshBuf()
{
    glDeleteBuffers(1, &vertexBuf_); vertexBuf_ = 0; nVertices_ = 0;
    glDeleteBuffers(1, &indexBuf_); indexBuf_ = 0; nIndices_ = 0;
}

MeshBuf::operator bool() const
{
    return (vertexBuf_ != 0 && nVertices_ != 0) &&
           (indexBuf_ != 0 && nIndices_ != 0);
}


/// Invokes `glBufferData` or `glBufferSubData` appropriately depending on
/// `oldSize <=> newSize`. `oldSize` can be zero.
static void updateGlBuffer(GLenum target, GLsizei oldSize, GLsizei newSize,
                           const void* data, GLenum usage)
{
    if(oldSize != newSize)
    {
        glBufferData(target, newSize, data, usage);
    }
    else
    {
        glBufferSubData(target, 0, newSize, data);
    }
}

bool MeshBuf::upload(const Mesh& mesh, GLenum usage)
{
    if(!vertexBuf_ || !indexBuf_)
    {
        return false;
    }

    { // Vertex buffer
        GLsizei oldVertexSize = nVertices_ * sizeof(Mesh::Vertex);
        GLsizei newVertexSize = mesh.vertices().size() * sizeof(Mesh::Vertex);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuf_);
        updateGlBuffer(GL_ARRAY_BUFFER, oldVertexSize, newVertexSize,
                       mesh.vertexData(), usage);
        nVertices_ = mesh.vertices().size();
    }

    { // Index buffer
        GLsizei oldIndexSize = nIndices_ * sizeof(Mesh::Index);
        GLsizei newIndexSize = mesh.indices().size() * sizeof(Mesh::Index);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf_);
        updateGlBuffer(GL_ELEMENT_ARRAY_BUFFER, oldIndexSize, newIndexSize,
                       mesh.indexData(), usage);
        nIndices_ = mesh.indices().size();
    }

    return true;
}

}
}
