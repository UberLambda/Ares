#pragma once

#include <Core/Api.h>
#include <Core/Data/ResourceParser.hh>
#include <string>

namespace Ares
{

/// The source code of a shader program.
/// Source code is always in GLSL 330 core syntax.
struct ARES_API ShaderSrc
{
    /// The source code of the vertex shader.
    /// Leave empty if unused.
    std::string vert;

    /// The source code of the fragment shader.
    /// Leave empty if unused.
    std::string frag;

    /// The source code of the geometry shader.
    /// Leave empty if unused.
    std::string geom;

    /// The source code of the tessellation evaluation shader.
    /// Leave empty if unused.
    std::string tes;

    /// The source code of the tessellation control shader.
    /// Leave empty if unused.
    std::string tcs;
};


/// Implementation of `ResourceParser` for `ShaderSrc`s.
template <>
struct ARES_API ResourceParser<ShaderSrc>
{
    static ErrString parse(ShaderSrc& outSrc, std::istream& stream, const Path& path,
                           ResourceLoader& loader);
};

}
