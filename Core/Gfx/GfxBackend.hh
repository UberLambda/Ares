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

/// A rendering backend for a `GfxRenderer`.
///
/// Partially inspired by OpenGL and sokol_gfx's design.
class GfxBackend
{
public:
    /// Creates a backend **that is capable out-of-the-box to generate, edit and
    /// destroy resources**, but won't be able to run commands until `init()` is called.
    ///
    /// Resources need to be able to be created so that the user can set up the
    /// `GfxPipeline` passed to `init()` properly.
    GfxBackend() = default;

    /// Initializes the rendering capabilities of the backend, setting command
    /// rendering up with the given pipeline.
    /// Can be called again to reinit the backend with a different pipeline.
    ///
    /// This needs **NOT** to destroy any resource when called, even if the pipeline
    /// has changed; this is because render targets and shaders for the pipeline
    /// are created by the user before he calls `init()`, so every resource is
    /// potentially needed for the new pipeline. It's the user's responsibility
    /// to get rid of leftover resources before calling `init()` again.
    virtual ErrString init(Ref<GfxPipeline> pipeline) = 0;

    /// Destroys the backend and all of it leftover resources (i.e. resources
    /// that haven't been `del*`'d).
    virtual ~GfxBackend() = default;


    /// Attempts to create a buffer given its description; returns a null handle on error.
    virtual Handle<GfxBuffer> genBuffer(const GfxBufferDesc& desc) = 0;

    /// Resizes the given buffer. Does nothing if the given handle is null or invalid.
    /// **This can invalidate any old data; call `editBuffer()` afterwards to
    /// reload it into the buffer if needed!**
    virtual void resizeBuffer(Handle<GfxBuffer> buffer, size_t newSize) = 0;

    /// Edits `dataSize` bytes in the given buffer, starting at `dataOffset`.
    /// Does nothing if the given handle is null or invalid or the data is too
    /// big / its offset too large to have it fit the buffer.
    virtual void editBuffer(Handle<GfxBuffer> buffer, size_t dataOffset, size_t dataSize, const void* data) = 0;

    /// Deletes the given buffer.
    /// Does nothing if the given handle is null or invalid.
    virtual void delBuffer(Handle<GfxBuffer> buffer) = 0;


    /// Attempts to create a texture given its description; returns a null handle on error.
    virtual Handle<GfxTexture> genTexture(const GfxTextureDesc& desc) = 0;

    /// Resizes the given texture. Does nothing if the given handle is null or invalid.
    /// **This can invalidate any old data; call `editTexture()` afterwards to
    /// reload it into the texture if needed!**
    /// Does nothing if the given handle is invalid.
    virtual void resizeTexture(Handle<GfxTexture> texture, Resolution newResolution) = 0;

    /// Edits the texture data inside `dataRect` in the texture to be `data`.
    /// Does nothing if the given handle is null or invalid or `dataRect` is too
    /// big / goes out of the texture's bounds.
    ///
    /// The type of the data to be supplied in `data` depends on the `dataType`
    /// in the `GfxTextureDesc` that was passed to `genTexture()` when the texture was created.
    virtual void editTexture(Handle<GfxTexture> texture, ViewRect dataRect, const void* data) = 0;

    /// Deletes the given texture.
    /// Does nothing if the given handle is null or invalid.
    virtual void delTexture(Handle<GfxTexture> texture) = 0;


    /// Attempts to create a shader given its description. Returns a null handle
    /// and sets `err` to contain a proper error message (if `err != nullptr`)
    /// on error.
    virtual Handle<GfxShader> genShader(const GfxShaderDesc& desc, ErrString* err=nullptr) = 0;

    /// Deletes the given shader.
    /// Does nothing if the given handle is null or invalid.
    virtual void delShader(Handle<GfxShader> shader) = 0;


    /// A callback for when the rendering resolution needs to be changed.
    ///
    /// Use this to setup the new viewport, recreate swapchains if needed, etc.
    virtual void changeResolution(Resolution resolution) = 0;

    /// Runs `cmds[cmdsOrder[i].index]` for each `i` in `[0..n)` in sequence.
    virtual void runCmds(const GfxCmd* cmds, const GfxCmdIndex* cmdsOrder, size_t n) = 0;
};

}
