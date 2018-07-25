#pragma once

#include <istream>
#include <Core/Data/Path.hh>

namespace Ares
{

/// The interface of a file store.
class FileStore
{
public:
    virtual ~FileStore() = default;

    /// Gets a pointer to the file at `path` in the file store, or null if it
    /// does not exist. Free it with `freeStream` after use.
    ///
    /// This operation should be thread safe.
    virtual std::istream* getStream(const Path& path) = 0;

    /// Frees the given stream back into the store.
    /// Does nothing if `stream` is null, was not returned by `getStream()`, or
    /// has already been freed.
    ///
    /// This operation should be thread safe.
    virtual void freeStream(std::istream* stream) = 0;
};

}
