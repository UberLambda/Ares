#pragma once

#include "../Base/Ref.hh"
#include "../Module/Module.hh"
#include "../Visual/Resolution.hh"
#include "GfxBackend.hh"

namespace Ares
{

class Window; // (#include "../Visual/Window.hh")
class GfxBackend; // (#include "Gfx/GfxBackend.hh")
class GfxRenderer; // (#include "Gfx/GfxRenderer.hh")

/// A graphics + graphical input module.
class GfxModule : public Module
{
    Window* window_; // (retrieved from `core.g().facilities` on init)
    Ref<GfxBackend> backend_; // (initialized/destroyed by `GfxModule`)
    GfxRenderer* renderer_; // (initialized/destroyed by `GfxModule`)

    /// Attempts to initialize OpenGL, returns `false` on error.
    /// `window_` should be inited for OpenGL 3.3+.
    bool initGL(Core& core);

    /// Attempts to initialize the renderer given its initial resolution.
    /// Returns `false` on error.
    bool initRenderer(Core& core, Resolution resolution);

public:
    GfxModule();
    ~GfxModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
