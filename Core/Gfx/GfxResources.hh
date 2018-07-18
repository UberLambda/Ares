#pragma once

#include "../Base/Handle.hh"
#include "../Resource/ShaderSrc.hh"
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
    enum Filter
    {
        Nearest = 0,
        Bilinear = 1,
        Trilinear = 2,
        Anisotropic = 3,
    };
    using Ch = ImageFormat::Channel;

    Resolution resolution{0, 0};
    ImageFormat format{Ch::None, Ch::None, Ch::None, Ch::None};
    GfxUsage usage = GfxUsage::Static;
    Filter minFilter = Nearest, magFilter = Nearest;

    enum class DataType
    {
        U8 = 0,
        U16 = 1,
        F32 = 2,
    } dataType = DataType::U8;
    const void* data = nullptr;
};

struct GfxShader;
struct GfxShaderDesc
{
    ShaderSrc src;
};

}
