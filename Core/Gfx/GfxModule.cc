#include "GfxModule.hh"

#include <utility>
#include <flextGL.h>
#include "GLFW/glfw3.h"
#include "../Core.hh"
#include "../Debug/Log.hh"
#include "../Data/ResourceLoader.hh"
#include "../Visual/Window.hh"

// FIXME TEST CODE - Remove this
#include "GL33/Shader.hh"
#include "../Data/Config.hh"
// /FIXME

namespace Ares
{

struct GfxModule::RenderData
{
    // FIXME TEST CODE - Remove this
    GLuint testVAO;
    GLuint testProgram;
    // /FIXME
};


GfxModule::GfxModule()
    : renderData_(nullptr), window_(nullptr)
{
}

#define glog (*core.g().log)

bool GfxModule::initWindow(Core& core)
{
    // FIXME: The `Window` is to be shared between graphics and input, so it should
    //        probably be managed by the `Core` in some way (like `requestFeature<Window>()`?)
    //        Window creation code should *NOT* be here!
    // TODO: Load videomode and title (app name) from config file
    VideoMode targetVideoMode;
    targetVideoMode.fullscreenMode = VideoMode::Windowed;
    targetVideoMode.resolution = {800, 600};
    targetVideoMode.refreshRate = 0; // (don't care)

    ARES_log(glog, Trace, "Creating window");

    Window window(Window::GL33, targetVideoMode, "Ares");
    if(!window)
    {
        ARES_log(glog, Fatal, "Failed to create window");
        return false;
    }

    window_ = new Window(std::move(window));
    return true;
}

bool GfxModule::initGL(Core& core)
{
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

bool GfxModule::init(Core& core)
{
    renderData_ = new RenderData();

    if(!initWindow(core))
    {
        return false;
    }

    if(!initGL(core))
    {
        return false;
    }

    // FIXME TEST CODE - Remove
    Ref<Config> cfgRef;
    auto err = core.g().resLoader->load<Config>(cfgRef, "TestScreen.armat");
    if(err)
    {
        ARES_log(glog, Error, "Config load error: %s", err);
        return false;
    }
    const Config& cfg = *cfgRef;

    glGenVertexArrays(1, &renderData_->testVAO); // FIXME VAO LEAKED!
    glBindVertexArray(renderData_->testVAO);

    const auto& vertSrc = cfg.get("shaders.vertSrc").value.string;
    if(vertSrc.empty())
    {
        ARES_log(glog, Fatal, "Failed to load test vertex shader");
        return false;
    }

    const auto& fragSrc = cfg.get("shaders.fragSrc").value.string;
    if(fragSrc.empty())
    {
        ARES_log(glog, Fatal, "Failed to load test fragment shader");
        return false;
    }

    GLuint testShaders[2];
    auto vertErr = GL33::compileShader(testShaders[0], GL_VERTEX_SHADER, vertSrc.c_str());
    if(vertErr)
    {
        ARES_log(glog, Fatal,
                 "Failed to compile test vertex shader: %s",
                 vertErr);
        return false;
    }
    auto fragErr = GL33::compileShader(testShaders[1], GL_FRAGMENT_SHADER, fragSrc.c_str());
    if(fragErr)
    {
        ARES_log(glog, Fatal,
                 "Failed to compile test fragment shader: %s",
                 fragErr);
        return false;
    }

    auto linkErr = GL33::linkShaderProgram(renderData_->testProgram,
                                           testShaders, testShaders + 2);
    if(linkErr)
    {
        ARES_log(glog, Fatal,
                 "Failed to link test shader program: %s",
                 linkErr);
    }

    glDeleteShader(testShaders[0]);
    glDeleteShader(testShaders[1]);

    // /FIXME

    return true;
}

void GfxModule::halt(Core& core)
{
    // FIXME TEST CODE - Remove
    glDeleteProgram(renderData_->testProgram); renderData_->testProgram = 0;
    glDeleteVertexArrays(1, &renderData_->testVAO); renderData_->testVAO = 0;
    // /FIXME

    delete window_; window_ = nullptr;
    delete renderData_; renderData_ = nullptr;
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

    // FIXME IMPLEMENT: Run rendering commands for the previous frame here

    // FIXME TEST CODE - Remove
    auto resolution = window_->resolution();
    glViewport(0, 0, resolution.width, resolution.height);

    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderData_->testProgram);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // /FIXME

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
