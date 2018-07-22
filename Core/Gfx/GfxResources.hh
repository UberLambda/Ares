#pragma once

#include "../Base/Handle.hh"
#include "../Base/Ref.hh"
#include "../Resource/ShaderSrc.hh"
#include "../Visual/Resolution.hh"
#include "../Visual/Resolution.hh"
#include "ImageFormat.hh"

namespace Ares
{

enum class GfxUsage
{
    Static = 0,
    Dynamic = 1,
    Streaming = 2, // Used for textures changed each frame, exp. if they are a Pass::target
};

struct GfxBuffer;
struct GfxBufferDesc
{
    size_t size = 0;
    const void* data = nullptr;
    GfxUsage usage = GfxUsage::Static;
};

struct GfxTexture;
struct GfxTextureDesc
{
    using Ch = ImageFormat::Channel;

    enum Type
    {
        _2D = 0,
        _2DArray = 1,
        _3D = 2,
        Cubemap = 3,
    } type = _2D;

    Resolution resolution{0, 0};
    size_t depth = 1;
    ImageFormat format{Ch::None, Ch::None, Ch::None, Ch::None};
    GfxUsage usage = GfxUsage::Static;

    enum Filter
    {
        Nearest = 0,
        Bilinear = 1,
        Trilinear = 2,
        Anisotropic = 3,
    } minFilter = Nearest, magFilter = Nearest;

    enum class DataType
    {
        U8 = 0,
        U16 = 1,
        F32 = 2,
    } dataType = DataType::U8;

    /// - `Type::_2D`: Rows of pixel data, top-to-bottom
    /// - `Type::_2DArray`: `depth` 2D images of resolution `resolution`, one after the other
    /// - `Type::_3D`: Same as `Type::_2DArray`, images are front (U=0.0) first, back (U=1.0) last
    /// - `Type::Cubemap`: 6 images of resolution `resolution` one after the other:
    ///     * +X
    ///     * -X
    ///     * +Y
    ///     * -Y
    ///     * +Z
    ///     * -Z
    const void* data = nullptr;
};

struct GfxShader;
struct GfxShaderDesc
{
    Ref<ShaderSrc> src;
};

}
