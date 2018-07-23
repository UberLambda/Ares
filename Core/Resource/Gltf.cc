#include "Gltf.hh"

#include <stddef.h>
#include <string.h>
#include <unordered_map>
#include "../Base/NumTypes.hh"
#include "../Data/ResourceLoader.hh"
#include "../Data/Path.hh"

namespace Ares
{

/// User data passed to tinygltf's FS callbacks.
struct FsData
{
    /// The parent file store.
    Ref<FileStore> fileStore;

    /// A map of "resource path" -> "resource file stream (or null if file missing)"
    std::unordered_map<Path, std::istream*> fileStreams;


    /// Frees all `fileStreams` loaded from `fileStore`.
    ~FsData()
    {
        for(auto it = fileStreams.begin(); it != fileStreams.end(); it ++)
        {
            fileStore->freeStream(it->second);
        }
    }
};

/// A `tinygltf::ReadWholeFileFunction` that gets passed a `FsData`.
/// The file stream gotten from the `FileStore` - even if null - is cached in `FsData`
/// itself.
static bool fsFileExists(const std::string& absFilepath, void* fsDataPtr)
{
    auto fsData = reinterpret_cast<FsData*>(fsDataPtr);

    auto it = fsData->fileStreams.find(absFilepath);
    if(it == fsData->fileStreams.end())
    {
        // This file was not checked, try to get a stream to it. On failure,
        // the stream will be null but it will be added to the list anyways to
        // signal that the file was checked
        std::istream* stream = fsData->fileStore->getStream(absFilepath);
        it = fsData->fileStreams.insert(std::make_pair(absFilepath, stream)).first;
    }

    return it->second != nullptr;
}

/// A `tinygltf::ExpandFilePathFunction` that gets passed a `FsData`.
static std::string fsExpandFilePath(const std::string& filepath, void* fsDataPtr)
{
    // Don't do any path expansion
    return filepath;
}

/// A `tinygltf::ReadWholeFileFunction` that gets passed a `FsData`.
static bool fsReadWholeFile(std::vector<unsigned char>* out, std::string* err,
                            const std::string& filepath,
                            void* fsDataPtr)
{
    // Cache the stream to the file at `filepath` if not already done; then check
    // if the stream is not null
    if(!fsFileExists(filepath, fsDataPtr))
    {
        if(err)
        {
          *err = "File not found";
        }
        return false;
    }

    // If the stream is not null we can try to read it
    auto fsData = reinterpret_cast<FsData*>(fsDataPtr);
    std::istream* stream = fsData->fileStreams[filepath];

    stream->seekg(0, std::ios::end);
    out->resize(stream->tellg());
    stream->seekg(0, std::ios::beg);

    stream->read(reinterpret_cast<char*>(&out[0]), out->size());
    // FIXME `istream::operator bool()` returns false here on Windows with no apparent reason
    //if(!(*stream))
    //{
    //  if(err)
    //  {
    //      *err = "File read error";
    //  }
    //  return false;
    //}

    return true;
}

/// An implementation of tinygltf's `WriteWholeFile` FS callback hooked to a
/// ResourceLoader.
static bool fsWriteWholeFile(std::string* err, const std::string& filepath,
                             const std::vector<unsigned char>& contents,
                             void* fsDataPtr)
{
    // TODO IMPLEMENT? No need to write the file anyways, Gltf resources are
    //                 readonly in Ares
    return true;
}


ErrString ResourceParser<Gltf>::parse(Gltf& outGltf,
                                      std::istream& stream, const Path& path,
                                      ResourceLoader& loader)
{
    const char* ext = path.extension();
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


    tinygltf::TinyGLTF tgltf;
    static const std::string tgltfRoot = ""; // The root dir of gltf external files,
                                             // relative to the `FileStore`'s root

    // Make the `TinyGLTF` use `ResourceLoader` for loading any external files
    FsData fsData;
    fsData.fileStore = loader.fileStore();

    tinygltf::FsCallbacks tgltfFs;
    tgltfFs.FileExists = &fsFileExists;
    tgltfFs.ExpandFilePath = &fsExpandFilePath;
    tgltfFs.ReadWholeFile = &fsReadWholeFile;
    tgltfFs.WriteWholeFile = &fsWriteWholeFile;
    tgltfFs.user_data = &fsData;

    tgltf.SetFsCallbacks(tgltfFs);

    // Actually try to load the file
    std::vector<unsigned char> data;

    stream.seekg(0, std::ios::end);
    data.resize(stream.tellg());
    stream.seekg(0, std::ios::beg);

    // FIXME Could error out on Windows
    stream.read(reinterpret_cast<char*>(&data[0]), data.size());
    //if(!stream)
    //{
    //    return ErrString("File read error");
    //}

    bool ok;
    std::string tgltfErr;

    if(isBinaryGltf)
    {
        ok = tgltf.LoadBinaryFromMemory(&outGltf.model(), &tgltfErr,
                                        &data[0], data.size(),
                                        tgltfRoot);
    }
    else
    {
        ok = tgltf.LoadASCIIFromString(&outGltf.model(), &tgltfErr,
                                       reinterpret_cast<const char*>(&data[0]),
                                       data.size(),
                                       tgltfRoot);
    }

    if(ok)
    {
        return {};
    }
    else
    {
        return ErrString("gltf error: ") + tgltfErr;
    }

    // (`~FsData()` will free all streams back to the `FileStore`)
}


/// Copies `n` `T`s from src to `dest`, casting each from `T` to `U`.
template <typename U, typename T>
inline static void castcpy(U* dest, const T* src, size_t n)
{
    for(size_t i = 0; i < n; i ++)
    {
        *(dest ++) = static_cast<T>(*(src ++));
    }
}


struct GltfAttribMeta
{
    size_t vtxOffset;
    size_t vtxSize;
    unsigned int nComponents;
};

#define ARES_meshVertexMeta(name) \
    offsetof(Mesh::Vertex, name), sizeof(Mesh::Vertex::name)

static const std::unordered_map<std::string, GltfAttribMeta> GLTF_MESH_ATTRIB_META_MAP =
{
    {"POSITION", {ARES_meshVertexMeta(position), 3}},
    {"NORMAL", {ARES_meshVertexMeta(normal), 3}},
    {"TANGENT", {ARES_meshVertexMeta(tangent), 4}},
    {"TEXCOORD_0", {ARES_meshVertexMeta(texCoord0), 2}},
    {"TEXCOORD_1", {ARES_meshVertexMeta(texCoord1), 2}},
    {"COLOR_0", {ARES_meshVertexMeta(color0), 4}},
    // TODO: JOINTS_0, WEIGHTS_0
};

Ref<Mesh> Gltf::extractMesh(unsigned int index) const
{
    if(index > model_.meshes.size())
    {
        return {};
    }

    const tinygltf::Mesh& tgfMesh = model_.meshes[index];

    if(tgfMesh.primitives.size() > 1 // More than one primitive
       || tgfMesh.targets.size() > 1) // More than ome morph target
    {
        // FIXME IMPLEMENT Unsupported GLTF mesh.
        return {};
    }

    Ref<Mesh> outMesh = makeRef<Mesh>();

    const tinygltf::Primitive& tgfPrimitive = tgfMesh.primitives[0];

    // Iterate every "attribute name -> accessor index" pair in the primitive
    for(const auto& accessorPair : tgfPrimitive.attributes)
    {
        auto attribMetaPair = GLTF_MESH_ATTRIB_META_MAP.find(accessorPair.first);
        if(attribMetaPair == GLTF_MESH_ATTRIB_META_MAP.end())
        {
            // Unrecognized attribute. This attribute is in the GLTF file, but
            // could not possibly be put in `Ares::Mesh::Vertex`; just ignore it
            continue;
        }

        // FIXME Check that the primitive mode is GL_TRIANGLES, convert other formats
        //       if needed

        const GltfAttribMeta& attribMeta = attribMetaPair->second;

        int accessorIndex = accessorPair.second;
        const tinygltf::Accessor& accessor = model_.accessors[accessorIndex];

        const tinygltf::BufferView& bufferView = model_.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model_.buffers[bufferView.buffer];

        const U8* tgfAttribData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        size_t tgfAttribStride = accessor.ByteStride(bufferView);

        // Read all data to outMesh->vertices for this attribute
        if(outMesh->vertices().size() < accessor.count)
        {
            // NOTE: All accessors for the same primitive's attributes should have
            //       the same `count` so the vector should only be resized once,
            //       when the first accessor's data is copied over
            outMesh->vertices().resize(accessor.count);
        }

        // NOTE `tinygltf::GetTypeSizeInBytes()` actually returns the number of
        //      components in a {`SCALAR`, `VEC2`, `VEC3`...), not the size in bytes!
        if(unsigned(tinygltf::GetTypeSizeInBytes(accessor.type)) > attribMeta.nComponents)
        {
            // Attribute has more components than what vertex can store;
            // silently skip it
            // TODO Maybe actually error out (return a null mesh handle) instead?
            continue;
        }

        // Copy attributes if they are already float; if they are unsigned ints,
        // unnormalize them back to floats in the 0..1 range
        // TODO NOTE That JOINTS_0 is **un**normalized U16s or U8s!
        constexpr const size_t attribStride = sizeof(Mesh::Vertex);
        const U8* inData = tgfAttribData;
        U8* outData = reinterpret_cast<U8*>(outMesh->vertices().data()) + attribMeta.vtxOffset;
        switch(accessor.componentType)
        {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
            {
                // src: vector of F32 -> dest: vector of F32
                size_t attribSize = attribMeta.nComponents * sizeof(float);
                for(size_t i = 0; i < accessor.count; i ++)
                {
                    memcpy(outData, inData, attribSize);
                    outData += attribStride;
                    inData += tgfAttribStride;
                }
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                // src: vector of UN16 -> dest: vector of F32
                for(size_t i = 0; i < accessor.count; i ++)
                {
                    auto outFloats = reinterpret_cast<float*>(outData);
                    auto inU16s = reinterpret_cast<const U16*>(inData);
                    for(size_t n = 0; n < attribMeta.nComponents; n ++)
                    {
                        *(outFloats ++) = float(*(inU16s ++)) / float(UINT16_MAX);
                    }
                    outData += attribStride;
                    inData += tgfAttribStride;
                }
            }
            break;
            default: // TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE
            {
                // src: vector of UN8 -> dest: vector of F32
                for(size_t i = 0; i < accessor.count; i ++)
                {
                    auto outFloats = reinterpret_cast<float*>(outData);
                    auto inU8s = /*reinterpret_cast<const U8*>*/(inData);
                    for(size_t n = 0; n < attribMeta.nComponents; n ++)
                    {
                        *(outFloats ++) = float(*(inU8s ++)) / float(UINT8_MAX);
                    }
                    outData += attribStride;
                    inData += tgfAttribStride;
                }
            }
            break;
        }
    }


    if(tgfPrimitive.indices >= 0) // Have `indices` accessor?
    {
        const tinygltf::Accessor& idxsAccessor = model_.accessors[tgfPrimitive.indices];
        const tinygltf::BufferView& idxsBufferView = model_.bufferViews[idxsAccessor.bufferView];
        const tinygltf::Buffer& idxsBuffer = model_.buffers[idxsBufferView.buffer];
        const U8* idxsData = idxsBuffer.data.data() + idxsBufferView.byteOffset + idxsAccessor.byteOffset;

        outMesh->indices().resize(idxsAccessor.count);

        switch(idxsAccessor.componentType)
        {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            {
                castcpy<Mesh::Index, U8>(outMesh->indices().data(), idxsData,
                                         idxsAccessor.count);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                // FIXME `reinterpret_cast` does not work for big endian platforms in this case
                auto u16IdxData = reinterpret_cast<const U16*>(idxsData);
                castcpy<Mesh::Index, U16>(outMesh->indices().data(), u16IdxData,
                                         idxsAccessor.count);
            }
            break;
            default: // TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT
            {
                // FIXME `assign()` does not work for big endian platforms in this case or if
                //       Index != U32
                auto u32IdxData = reinterpret_cast<const U32*>(idxsData);
                outMesh->indices().assign(u32IdxData, u32IdxData + idxsAccessor.count);
            }
            break;
        }
    }

    return outMesh;
}

Ref<Mesh> Gltf::extractMesh(const char* name) const
{
    if(name == nullptr)
    {
        return {};
    }

    unsigned int i = 0;
    for(auto it = model_.meshes.begin(); it != model_.meshes.end(); it ++, i ++)
    {
        if(it->name == name) // (implicit `strcmp()`)
        {
            return extractMesh(i);
        }
    }

    return {};
}

}
