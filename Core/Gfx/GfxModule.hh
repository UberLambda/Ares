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
    Ref<GfxBackend> backend_; // (initialized/destroyed by `GfxModule`)
    Ref<GfxPipeline> pipeline_; // (initialized/destroyed by `GfxModule`)
    GfxRenderer* renderer_; // (initialized/destroyed by `GfxModule`)

    /// Attempts to initialize OpenGL, returns `false` on error.
    /// `window_` should be inited for OpenGL 3.3+.
    bool initGL(Core& core);

    /// Creates `renderer_`.
    /// Returns `false` on error.
    /// A graphics context should already be inited.
    bool createRenderer(Core& core);

    /// Attempts to create `pipeline_`.
    /// Returns `false` on error.
    /// `renderer_` should already be created, **but not inited yet**
    bool createPipeline(Core& core, Resolution resolution);

    /// Attempts to initialize `renderer_` after it and `pipeline_` have been created.
    /// Returns `false` on error.
    /// `renderer_` and `pipeline_` should already be created.
    bool initPipelineAndRenderer(Core& core);

public:
    GfxModule();
    ~GfxModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
