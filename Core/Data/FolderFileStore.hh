#pragma once

#include <fstream>
#include <sstream>
#include "FileStore.hh"

namespace Ares
{

/// A `FileStore` that loads files from a local folder.
class FolderFileStore : public FileStore
{
    Path root_;

public:
    FolderFileStore(const Path& root)
        : root_(root)
    {
    }

    std::istream* getStream(const Path& path) override
    {
        // FIXME IMPORTANT Security hazard, path could contain ".."!!
        std::ostringstream fullPathBuilder;
        fullPathBuilder << root_ << '/' << path;
        Path fullPath = std::move(fullPathBuilder.str());

        auto stream = new std::ifstream(fullPath);
        if(!(*stream))
        {
            delete stream; stream = nullptr;
        }
        return stream;
    }

    void freeStream(std::istream* stream) override
    {
        delete stream;
    }


    /// Returns the path to the root of the folder file store.
    inline const Path& root() const
    {
        return root_;
    }
};

}
