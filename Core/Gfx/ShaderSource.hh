#pragma once

#include "../Data/PlainText.hh"

namespace Ares
{

/// The source code of a shader program.
/// The various sources are to be written in GLSL 330 core, that is
/// compiled/transpiled to other shader formats (like SPIR-V) if needed.
class ShaderSource
{
    PlainText vertSrc; ///< Th code for the vertex shader, empty if unused.
    PlainText fragSrc; ///< The code for the fragment shader, empty if unused.
    PlainText geomSrc; ///< The code for the geometry shader, empty if unused.
};

}
