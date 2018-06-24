#pragma once

#include "Shader.hh"

namespace Ares
{

/// An high-level material.
struct Material
{
    Shader shader; ///< The shader program used to render the material.
};

}
