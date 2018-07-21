#pragma once

#include <string.h>
#define TINYGLTF_NO_FS
#include <tiny_gltf.h>
#include "../Base/Ref.hh"
#include "../Data/ResourceParser.hh"
#include "../Data/PlainText.hh"
#include "Mesh.hh"

namespace Ares
{

/// A GLTF 2.0 file.
/// .gltf/.glb files can contain meshes, textures, animations, materials,
/// and/or entire scene graphs.
class Gltf
{
    tinygltf::Model model_;

public:
    Gltf()
    {
    }

    Gltf(tinygltf::Model&& model)
        : model_(std::move(model))
    {
    }


    /// Returns the underlying `tinygltf::Model`.
    inline tinygltf::Model& model()
    {
        return model_;
    }
    inline const tinygltf::Model& model() const
    {
        return model_;
    }

    /// An alias of `model()` for convenience.
    inline tinygltf::Model* operator->()
    {
        return &model_;
    }
    inline const tinygltf::Model* operator->() const
    {
        return &model_;
    }


    /// Returns a new `Mesh` constructed from the mesh data at the given index
    /// or with a specific name in `model()`, or an empty ref if no such mesh could be found.
    Ref<Mesh> extractMesh(unsigned int index) const;
    Ref<Mesh> extractMesh(const char* name) const;

    // TODO IMPLEMENT ways to extract images, materials, skeletons, animations,
    //                and a whole Entity scenegraph from nodes
};


/// Implementation of `ResourceParser` for `Gltf`s.
template <>
struct ResourceParser<Gltf>
{
    static ErrString parse(Gltf& outGltf, std::istream& stream, const Path& path,
                           ResourceLoader& loader);
};

}
