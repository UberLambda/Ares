#pragma once

#include "../Module/Module.hh"
#include "../Visual/Resolution.hh"

namespace Ares
{

class Window; // (#include "../Visual/Window.hh")

/// A graphics + graphical input module.
class GfxModule : public Module
{
    Window* window_; // (retrieved from `core.g().facilities` on init)

    struct RenderData;
    RenderData* renderData_;

    /// Attempts to initialize OpenGL, returns `false` on error.
    /// `window_` should be inited for OpenGL 3.3+.
    bool initGL(Core& core);

    /// Attempts to initialize the G-Buffer given its initial resolution.
    /// Returns `false` on error.
    bool initGBuffer(Core& core, Resolution resolution);

public:
    GfxModule();
    ~GfxModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
