#pragma once

#include "../Data/PlainText.hh"

namespace Ares
{

/// A shader program.
class Shader
{
    PlainText vertSrc; ///< The GLSL vertex shader.
    PlainText fragSrc; ///< The GLSL fragment shader.
    PlainText geomSrc; ///< The GLSL geometry shader, empty if unused.
};

}
