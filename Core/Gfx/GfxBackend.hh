#pragma once

#include <stddef.h>
#include "../Base/Ref.hh"
#include "../Base/ErrString.hh"
#include "../Visual/ViewRect.hh"
#include "GfxResources.hh"
#include "GfxPipeline.hh"
#include "GfxCmd.hh"

namespace Ares
{

class GfxBackend
{
public:
    GfxBackend() = default;

    // Can be called again to reinit the backend with a different pipeline.
    // **Backends will be inited AFTER some `gen*()` calls on it are made, since
    //   the pipeline requires the generation of some textures and stuff!!**
    // **Hence the backend should be able to generate and delete resources AS SOON
    //   AS ITS CONSTRUCTOR IS RUN!**
    // Called by `GfxRenderer()`, do not call manually beforehand
    //
    // This needs **NOT** to destroy any resource when called, even if the pipeline has changed.
    // This is because render targets and shaders for the pipeline are created before
    // calling `init()` by the user and they could potentially get deleted along
    // with the leftovers of the previous pipeline on `init()`!
    virtual ErrString init(Ref<GfxPipeline> pipeline) = 0;

    /// Must destroy all leftover resources.
    virtual ~GfxBackend() = default;


    /// Returns 0 on error
    virtual Handle<GfxBuffer> genBuffer(const GfxBufferDesc& desc) = 0;

    /// Invalidates the old data.
    /// Does nothing if the given handle is invalid.
    virtual void resizeBuffer(Handle<GfxBuffer> buffer, size_t newSize) = 0;

    /// Does nothing if the given handle is invalid.
    virtual void editBuffer(Handle<GfxBuffer> buffer, size_t dataOffset, size_t dataSize, const void* data) = 0;

    /// Does nothing if the given handle is invalid.
    virtual void delBuffer(Handle<GfxBuffer> buffer) = 0;


    /// Returns 0 on error
    virtual Handle<GfxTexture> genTexture(const GfxTextureDesc& desc) = 0;

    /// Invalidates the old data.
    /// Does nothing if the given handle is invalid.
    virtual void resizeTexture(Handle<GfxTexture> texture, Resolution newResolution) = 0;

    /// The type of the data to be supplied in `data` depends on the `dataType`
    /// in the `GfxTextureDesc` that was passed to `genTexture()` when the texture was created.
    /// Does nothing if the given handle is invalid.
    virtual void editTexture(Handle<GfxTexture> texture, ViewRect dataRect, const void* data) = 0;

    /// Does nothing if the given handle is invalid.
    virtual void delTexture(Handle<GfxTexture> texture) = 0;


    /// Returns 0 and sets `err` on error
    virtual Handle<GfxShader> genShader(const GfxShaderDesc& desc, ErrString* err=nullptr) = 0;

    /// Does nothing if the given handle is invalid.
    virtual void delShader(Handle<GfxShader> shader) = 0;


    /// Run when the rendering resolution is changed.
    virtual void changeResolution(Resolution resolution) = 0;

    /// Runs `cmds[cmdsOrder[i].index]`` for each `i` in `[0..n)`.
    virtual void runCmds(const GfxCmd* cmds, const GfxCmdIndex* cmdsOrder, size_t n) = 0;
};

}
