#pragma once

#include "../Module/Module.hh"

namespace Ares
{

class Window; // (#include "../Visual/Window.hh")

/// A graphics + graphical input module.
class GfxModule : public Module
{
    Window* window_; // FIXME: See `GfxModule::init()` implementation

    struct RenderData;
    RenderData* renderData_;

    /// Attempts to initialize `window_`, returns `false` on error.
    // TODO Selectable rendering API for the window
    bool initWindow(Core& core);

    /// Attempts to initialize OpenGL, returns `false` on error.
    /// `window_` should be inited for OpenGL 3.3+.
    bool initGL(Core& core);

public:
    GfxModule();
    ~GfxModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
