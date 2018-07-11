#include "Gltf.hh"

#include <unordered_map>
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
    if(!(*stream))
    {
      if(err)
      {
          *err = "File read error";
      }
      return false;
    }

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

    stream.read(reinterpret_cast<char*>(&data[0]), data.size());
    if(!stream)
    {
        return ErrString("File read error");
    }

    bool ok;
    std::string tgltfErr;

    if(isBinaryGltf)
    {
        ok = tgltf.LoadBinaryFromMemory(&outGltf, &tgltfErr,
                                        &data[0], data.size(),
                                        tgltfRoot);
    }
    else
    {
        ok = tgltf.LoadASCIIFromString(&outGltf, &tgltfErr,
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

}
