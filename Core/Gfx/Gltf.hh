#pragma once

#include <string.h>
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
    static ErrString parse(Gltf& outGltf, std::istream& stream, const char* ext)
    {
        bool isBinaryGltf;
        if(strcmp(ext, ".glb") == 0)
        {
            isBinaryGltf = true;
        }
        else if(strcmp(ext, ".gltf") == 0)
        {
            isBinaryGltf = false;
        }
        else
        {
            return ErrString("Invalid extension for GLTF2 file: ") + ext;
        }

        PlainText data;
        ErrString dataReadErr = ResourceParser<PlainText>::parse(data, stream, ext);
        if(dataReadErr)
        {
            return dataReadErr;
        }

        tinygltf::TinyGLTF gltfLoader;
        bool gltfLoadedOk = false;
        std::string gltfLoadErr;

        // FIXME IMPORTANT tinygltf will try to load external files from the
        //                 physical filesystem, but they should be loaded from
        //                 `std::istream`s obtained from a `FileStore`!
        //                 See: https://github.com/syoyo/tinygltf/issues/77
        static const std::string fileRoot("");
        if(isBinaryGltf)
        {
            auto rawData = reinterpret_cast<unsigned char*>(&data[0]);
            auto rawDataSize = data.length();
            gltfLoadedOk = gltfLoader.LoadBinaryFromMemory(&outGltf, &gltfLoadErr,
                                                           rawData, rawDataSize, fileRoot);
        }
        else
        {
            gltfLoadedOk = gltfLoader.LoadASCIIFromString(&outGltf, &gltfLoadErr,
                                                          &data[0], data.length(), fileRoot);
        }

        if(gltfLoadedOk)
        {
            return {};
        }
        else
        {
            return ErrString(gltfLoadErr);
        }
    }
};

}
