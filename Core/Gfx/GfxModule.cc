#include "GfxModule.hh"

#include <utility>
#include "../Core.hh"
#include "../Debug/Log.hh"
#include "../Data/ResourceLoader.hh"
#include "../Visual/Window.hh"
#include "GfxRenderer.hh"
#include "GfxBackend.hh"

// OpenGL 3.3 core backend
#include <flextGL.h>
#include "GL33/Backend.hh"

namespace Ares
{

GfxModule::GfxModule()
    : window_(nullptr), renderer_(nullptr)
{
}

#define glog (*core.g().log)

bool GfxModule::initGL(Core& core)
{
    ARES_log(glog, Trace, "Initializing OpenGL");

    window_->beginFrame(); // Make OpenGL context current on this thread

    int majorVersion = -1, minorVersion = -1;
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);

    if(majorVersion <= 0 || minorVersion <= 0)
    {
        ARES_log(glog, Error,
                 "Could not query OpenGL version, context is too old or broken!");
        return false;
    }

    { // Some debug logging
        ARES_log(glog, Debug,
                 "Got OpenGL %d.%d [%s, %s]",
                 majorVersion, minorVersion, glGetString(GL_VERSION), glGetString(GL_VENDOR));

        static constexpr const char* checkStrs[] = { "no", "yes" };
#define ARES_gfxLogHasGL(func) \
        ARES_log(glog, Trace, "Have %s? %s", #func, checkStrs[func != nullptr]);

        ARES_gfxLogHasGL(glMultiDrawElementsIndirect);

#undef ARES_gfxLogHasGL
    }

    return true;
}

bool GfxModule::createRenderer(Core& core)
{
    ARES_log(glog, Trace, "Creating renderer (OpenGL 3.3 core)");

    backend_ = intoRef<GfxBackend>(new GL33::Backend());
    renderer_ = new GfxRenderer(backend_);

    return true;
}

bool GfxModule::createPipeline(Core& core, Resolution resolution)
{
    ARES_log(glog, Trace,
             "Creating GfxPipeline (initial resolution: %s)",
             resolution);

    pipeline_ = makeRef<GfxPipeline>();

    return true;
}

bool GfxModule::initPipelineAndRenderer(Core& core)
{
    // Initializes both the renderer and the backend for this pipeline
    ErrString err = renderer_->init(pipeline_);
    if(!err)
    {
        ARES_log(glog, Trace, "Renderer inited");
        return true;
    }
    else
    {
        ARES_log(glog, Error,
                 "Failed to initialize renderer: %s",
                 err);
        return false;
    }
}

bool GfxModule::init(Core& core)
{
    window_ = core.g().facilities.get<Window>();
    if(!window_ || !window_->operator bool())
    {
        ARES_log(glog, Error,
                 "GfxModule requires a Window facility but %s",
                 window_ ? "it failed to initialize" : "it was not added");
        return false;
    }

    Resolution initialResolution = window_->resolution();

    bool allOk = initGL(core)
                 && createRenderer(core)
                 && createPipeline(core, initialResolution)
                 && initPipelineAndRenderer(core);

    return allOk;
}

void GfxModule::halt(Core& core)
{
    ARES_log(glog, Trace, "Destroying GfxRenderer");
    delete renderer_; renderer_ = nullptr;

    ARES_log(glog, Trace, "Destroying GfxBackend");
    (void)backend_.~Ref(); // (will destroy all OpenGL data)

    window_ = nullptr; // (will be destroyed by `Core`)
}

GfxModule::~GfxModule()
{
}


void GfxModule::mainUpdate(Core& core)
{
    // Poll events for the current and/or next frame[s]
    // TODO: Move input polling somewhere else so that even if the rendering is
    //       lagging rendering won't suffer
    window_->pollEvents();

    // Execute rendering commands calculate for the previous frame
    // NOTE: *THIS IS ALWAYS ONE FRAME BEHIND!*
    //       Rendering commands for a frame are generated by worker threads by
    //       `updateTask()`, but `mainUpdate()` - that actually executes them -
    //       is run before `updateTask()`. This means that the rendering commands
    //       run will always be the ones generated for the previous frame,
    //       introducing a one-frame rendering lag
    window_->beginFrame();

    auto resolution = window_->resolution();
    renderer_->renderFrame(resolution);

    window_->endFrame();

    // FIXME TEST, USE A REAL EVENT SYSTEM TO TELL THE CORE TO QUIT!
    if(window_->quitRequested())
    {
        core.halt();
    }
}

Task GfxModule::updateTask(Core& core)
{
    auto updateFunc = [](TaskScheduler* scheduler, void* data)
    {
        // FIXME IMPLEMENT: Generate rendering commands for this frame here
    };
    return {updateFunc, this};
}

}
