#pragma once

#include "../Base/Ref.hh"
#include "../Module/Module.hh"
#include "../Visual/Resolution.hh"
#include "GfxBackend.hh"
#include "GfxPipeline.hh"

namespace Ares
{

class Window; // (#include "../Visual/Window.hh")
class GfxRenderer; // (#include "Gfx/GfxRenderer.hh")

/// A graphics + graphical input module.
class GfxModule : public Module
{
    Window* window_; // (retrieved from `core.g().facilities` on init)

    Resolution resolution_;
    Ref<GfxBackend> backend_; // (initialized/destroyed by `GfxModule`)
    Ref<GfxPipeline> pipeline_; // (initialized/destroyed by `GfxModule`)
    GfxRenderer* renderer_; // (initialized/destroyed by `GfxModule`)

    struct Data;
    Data* data_; // (initialized/destroyed by `GfxModule`)

    /// Attempts to initialize OpenGL, returns `false` on error.
    /// `window_` should be inited for OpenGL 3.3+.
    bool initGL(Core& core);

    /// Creates `renderer_`.
    /// Returns `false` on error.
    /// A graphics context should already be inited.
    bool createRenderer(Core& core);

    /// Creates a texture to be used as `pipeline_`'s target with the given
    /// resolution and image format.
    /// Returns `false` on error.
    /// `renderer_` and its backend should already be created.
    Handle<GfxTexture> createPipelineTarget(Core& core, Resolution resolution,
                                            ImageFormat format);

    /// Attempts to load a `ShaderSrc` resource at `path`, then compile the shader
    /// and return its handle.
    /// Returns a null handle and logs an error on failure.
    /// `renderer_` and its backend should already be created.
    Handle<GfxShader> loadShader(Core& core, const Path& path);

    /// Attempts to create `pipeline_`.
    /// Returns `false` on error.
    /// `renderer_` and its backend should already be created, **but not inited yet**
    bool createPipeline(Core& core, Resolution resolution);

    /// Attempts to initialize `renderer_` after it and `pipeline_` have been created.
    /// Returns `false` on error.
    /// `renderer_`, its backend and `pipeline_` should already be created.
    bool initPipelineAndRenderer(Core& core);


    /// Executed when the resolution of `window_`'s renderable area changes; resizes
    /// all of `pipeline_`'s render targets accordingly
    void changeResolution(Core& core, Resolution newResolution);

    /// Enqueue the `GfxCmd`s required to render Scene data (MeshComps, ...) for
    /// this frame into the renderer.
    void genSceneCmds(Core& core);

public:
    GfxModule();
    ~GfxModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
