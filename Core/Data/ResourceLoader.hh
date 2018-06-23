#pragma once

#include <istream>

namespace Ares
{

/// The interface of a loader of `T` resources.
template <typename T>
class ResourceLoader
{
public:
    virtual ~ResourceLoader() = default;

    /// Attempts to load `outResource` from the given stream; returns `false` on
    /// error. `ext` contains the file extension (lowercase and without the initial dot)
    /// that the resource would have if it were in a file.
    virtual bool load(T& outResource, std::istream& stream, const char* ext) = 0;
};

}
