#pragma once

#include "../Base/NumTypes.hh"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Ares
{

/// An unsigned 8-bit RGB color.
using RGB8 = glm::tvec3<U8>;

/// A 32-bit floating point RGB color.
using RGBF = glm::tvec3<F32>;


/// An unsigned 8-bit RGBA color.
using RGBA8 = glm::tvec4<U8>;

/// A 32-bit floating point RGBA color.
using RGBAF = glm::tvec4<F32>;

}
