#pragma once

#include <istream>

namespace Ares
{

/// The template to implement to have a parser of `T` resources
/// that a `ResourceLoader` can use.
template <typename T>
struct ResourceParser
{
    /// Attempts to parse `outResource` from the given stream; returns `false` on
    /// error. `ext` contains the file extension (lowercase and including the initial dot)
    /// that the data in the stream would have if it was in a file.
    static bool parse(T& outResource, std::istream& stream, const char* ext);
};

}
