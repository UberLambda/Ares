#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace Ares
{

/// An indexed triangle mesh.
class Mesh
{
public:
    /// A vertex of the mesh. See GLTF 2.0's per-vertex attributes.
    struct Vertex
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
    std::vector<Index> vertices_;

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
        return reinterpret_cast<float*>(&indices_[0]);
    }

    /// Returns the vertex index data as an array of indices.
    inline const Index* vertexData() const
    {
        return &indices_[0];
    }
};





}
