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

Handle<GfxTexture> GfxModule::createPipelineTarget(Core& core, Resolution resolution,
                                                   ImageFormat format)
{
    GfxTextureDesc desc;
    desc.resolution = resolution;
    desc.format = format;
    desc.usage = GfxUsage::Streaming;
    desc.minFilter = desc.magFilter = GfxTextureDesc::Filter::Nearest; // TODO Maybe bilinear?
    desc.data = nullptr;

    return renderer_->backend().genTexture(desc); // (Returns a 0 handle on error)
}

bool GfxModule::createPipeline(Core& core, Resolution resolution)
{
    ARES_log(glog, Trace,
             "Creating GfxPipeline (initial resolution: %s)",
             resolution);

    pipeline_ = makeRef<GfxPipeline>();
    GfxBackend* backend = renderer_->operator ->();


    using Pass = GfxPipeline::Pass;
    using VA = GfxPipeline::VertexAttrib;
    using Ch = ImageFormat::Channel;

    // #0: PBR pass
    // - Input attribs: matches `Mesh::Vertex`
    // - Shader: PBR.arsh
    // - Output targets:
    //      * Color (RGBA16F, linear): RGB = color and lighting data, A = alpha
    //      * Normals (RGB10A2, linear): RGB = Normals (rescaled from [-1..1] to [0..1])
    //      * RMID (RGBA8, linear): R = roughness, G = metallicity,
    //                              I+D = id of the entity packed as big endian U16
    //      * Depth (32F)
    Pass pbrPass;

    pbrPass.vertexAttribs[0] = {"position", VA::Type::F32, 3};
    pbrPass.vertexAttribs[1] = {"normal", VA::Type::F32, 3};
    pbrPass.vertexAttribs[2] = {"tangent", VA::Type::F32, 4};
    pbrPass.vertexAttribs[3] = {"texCoord0", VA::Type::F32, 2};
    pbrPass.vertexAttribs[4] = {"texCoord1", VA::Type::F32, 2};
    pbrPass.vertexAttribs[5] = {"color0", VA::Type::F32, 4};
    pbrPass.nVertexAttribs = 6;

    pbrPass.targets[0] = createPipelineTarget(core, resolution,
                                              {Ch::F16, Ch::F16, Ch::F16, Ch::F16});
    pbrPass.targets[1] = createPipelineTarget(core, resolution,
                                              {Ch::UN10, Ch::UN10, Ch::UN10, Ch::UN2});
    pbrPass.targets[2] = createPipelineTarget(core, resolution,
                                              {Ch::UN8, Ch::UN8, Ch::UN8, Ch::UN8});
    pbrPass.targets[3] = createPipelineTarget(core, resolution,
                                              {Ch::F32Depth});
    pbrPass.nTargets = 4;
    pbrPass.clearTargets = true;

    //pbrPass.shader = <>; // FIXME Load "PBR.arsh" ShaderSrc, `genShader()` it, set it here

    pipeline_->passes.push_back(pbrPass);

    // #1: Postprocess pass
    // - Input attribs: none, generates fullscreen triangles in the vertex shader directly
    // - Shader: Postprocess.arsh
    // - Output targets:
    //      * <Screen>
    Pass ppPass;

    ppPass.nVertexAttribs = 0;

    ppPass.targets[0] = GfxPipeline::Pass::SCREEN_TARGET;
    ppPass.nTargets = 1;
    pbrPass.clearTargets = false; // No need to, only rendering fullscreen triangles

    //pbrPass.shader = <>; // FIXME Load "Postprocess.arsh" ShaderSrc, `genShader()` it, set it here

    pipeline_->passes.push_back(ppPass);

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
