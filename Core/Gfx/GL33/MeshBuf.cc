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


void MeshBuf::setupVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuf_);

    // vec3 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3, GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          (void*)offsetof(Mesh::Vertex, position));
    // vec3 normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          3, GL_FLOAT,
                          GL_FALSE, // (TODO: normalize=on?)
                          sizeof(Mesh::Vertex),
                          (void*)offsetof(Mesh::Vertex, normal));
    // vec4 tangent
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,
                          4, GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          (void*)offsetof(Mesh::Vertex, tangent));
    // vec2 texCoord0
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,
                          2, GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          (void*)offsetof(Mesh::Vertex, texCoord0));
    // vec2 texCoord1
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4,
                          2, GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          (void*)offsetof(Mesh::Vertex, texCoord1));
    // vec4 color0
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5,
                          4, GL_FLOAT,
                          GL_FALSE,
                          sizeof(Mesh::Vertex),
                          (void*)offsetof(Mesh::Vertex, color0));
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
