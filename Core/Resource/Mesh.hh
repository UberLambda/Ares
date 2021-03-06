#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <Core/Api.h>

namespace Ares
{

/// An indexed triangle mesh.
class ARES_API Mesh
{
public:
    /// A vertex of the mesh. See GLTF 2.0's per-vertex attributes.
    struct ARES_API Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 tangent; ///< W component is either +1 or -1; see see GLTF 2.0's spec.
        glm::vec2 texCoord0;
        glm::vec2 texCoord1;
        glm::vec4 color0;
    };

    /// An index to a vertex of the mesh.
    using Index = unsigned int;

private:
    std::vector<Vertex> vertices_;
    std::vector<Index> indices_;

public:
    /// Gets/modifies the vertices currently in the mesh.
    inline std::vector<Vertex>& vertices()
    {
        return vertices_;
    }
    inline const std::vector<Vertex>& vertices() const
    {
        return vertices_;
    }

    /// Gets/modifies the vertex indices currently in the mesh.
    inline std::vector<Index>& indices()
    {
        return indices_;
    }
    inline const std::vector<Index>& indices() const
    {
        return indices_;
    }


    /// Returns the number of triangles in the mesh, i.e. the number of
    /// indices in it / 3.
    inline size_t nTriangles() const
    {
        return indices_.size() / 3;
    }

    /// Returns the vertex data as an array of floating point values.
    /// `Vertex` structures are implicitly convertible to arrays of floats.
    inline const float* vertexData() const
    {
        return reinterpret_cast<const float*>(vertices_.data());
    }

    /// Returns the size in bytes of the `vertexData()` array.
    inline size_t vertexDataSize() const
    {
        return vertices_.size() * sizeof(Vertex);
    }

    /// Returns the vertex index data as an array of indices.
    inline const Index* indexData() const
    {
        return indices_.data();
    }

    /// Returns the size in bytes of the `indexData()` array.
    inline size_t indexDataSize() const
    {
        return indices_.size() * sizeof(Index);
    }
};





}
