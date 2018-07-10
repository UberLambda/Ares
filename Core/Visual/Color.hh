#pragma once

#include "../Base/NumTypes.hh"
#include <glm/vec3.hh>
#include <glm/vec4.hh>

namespace Ares
{

/// An unsigned 8-bit RGB color.
using RGB8 = glm::vec3<U8>;

/// A 32-bit floating point RGB color.
using RGBF = glm::vec3<F32>;


/// An unsigned 8-bit RGBA color.
using RGBA8 = glm::vec4<U8>;

/// A 32-bit floating point RGBA color.
using RGBAF = glm::vec4<F32>;

}
