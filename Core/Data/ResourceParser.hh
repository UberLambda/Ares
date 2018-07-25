#pragma once

#include <istream>
#include <Core/Api.h>
#include <Core/Base/ErrString.hh>
#include <Core/Data/Path.hh>

namespace Ares
{

class ResourceLoader; // (#include "ResourceLoader/ResourceLoader.hh")

/// The template to implement to have a parser of `T` resources
/// that a `ResourceLoader` can use.
template <typename T>
struct ARES_API ResourceParser
{
    /// Attempts to parse `outResource` from the given stream; returns a non-empty
    /// error string on error. `path` contains the path to the resource file being
    /// parsed from `stream`.
    /// If required, resources can ask their parent `resourceLoader` to parse
    /// other resources that this one depends on. For example, think of a material
    /// resource requiring to load some textures.
    static ErrString parse(T& outResource, std::istream& stream, const Path& path,
                           ResourceLoader& loader);
};

}
