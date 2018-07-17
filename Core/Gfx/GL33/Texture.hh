#pragma once

#include <flextGL.h>
#include "../../Base/ErrString.hh"
#include "../ImageFormat.hh"

namespace Ares
{
namespace GL33
{

/// Returns an OpenGL format and internal format from an ImageFormat.
/// Returns `false` if the format could not be converted to a valid OpenGL texture
/// format.
bool textureFormats(GLenum& outFormat, GLenum& outInternalFormat,
                    ImageFormat imageFormat);

}
}
