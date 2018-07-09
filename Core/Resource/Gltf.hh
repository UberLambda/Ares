#pragma once

#include <string.h>
#define TINYGLTF_NO_FS
#include <tiny_gltf.h>
#include "../Data/ResourceParser.hh"
#include "../Data/PlainText.hh"

namespace Ares
{

/// A GLTF 2.0 file.
/// .gltf/.glb files can contain meshes, textures, animations, materials,
/// and/or entire scene graphs.
using Gltf = tinygltf::Model;


/// Implementation of `ResourceParser` for `Gltf`s.
template <>
struct ResourceParser<Gltf>
{
    static ErrString parse(Gltf& outGltf, std::istream& stream, const char* ext,
                           ResourceLoader& loader);
};

}
