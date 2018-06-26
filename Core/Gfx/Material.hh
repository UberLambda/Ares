#pragma once

#include <string>
#include "ShaderSource.hh"

namespace Ares
{

/// An high-level material.
struct Material
{
    std::string name; ///< The name of the material.
    std::string descr; ///< A description of the material.
    ShaderSource shaderSource; ///< The source code of the shader program used
                               ///  to render the material.
};

}
