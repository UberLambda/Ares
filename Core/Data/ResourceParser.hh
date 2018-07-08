#pragma once

#include <istream>
#include "../Base/ErrString.hh"

namespace Ares
{

class ResourceLoader; // (#include "ResourceLoader/ResourceLoader.hh")

/// The template to implement to have a parser of `T` resources
/// that a `ResourceLoader` can use.
template <typename T>
struct ResourceParser
{
    /// Attempts to parse `outResource` from the given stream; returns a non-empty
    /// error string on error. `ext` contains the file extension (lowercase and
    /// including the initial dot) that the data in the stream would have if it
    /// was in a file.
    /// If required, resources can ask their parent `resourceLoader` to parse
    /// other resources that this one depends on. For example, think of a material
    /// resource requiring to load some textures.
    static ErrString parse(T& outResource, std::istream& stream, const char* ext,
                           ResourceLoader& loader);
};

}
