#pragma once

#include <flextGL.h>
#include <Core/Base/ErrString.hh>
#include <Core/Base/NumTypes.hh>
#include <Core/Visual/Resolution.hh>
#include <Core/Gfx/ImageFormat.hh>

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
