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

#include "../Resource/Mesh.hh" // FIXME!! TEST!!
#include "GfxResources.hh" // FIXME!! TEST!!

namespace Ares
{

GfxModule::GfxModule()
    : window_(nullptr), resolution_{0, 0}, renderer_(nullptr)
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

Handle<GfxShader> GfxModule::loadShader(Core& core, const Path& path)
{
    ARES_log(glog, Trace, "Loading shader: %s", path);

    Ref<ShaderSrc> src;
    ErrString err = core.g().resLoader->load<ShaderSrc>(src, path);
    if(err)
    {
        ARES_log(glog, Error, "Failed loading shader source at %s: %s",
                 path, err);
        return Handle<GfxShader>();
    }

    GfxShaderDesc desc;
    desc.src = src;

    Handle<GfxShader> handle = renderer_->backend().genShader(desc, &err);
    if(!handle)
    {
        ARES_log(glog, Error, "Failed compiling shader at %s: %s",
                 path, err);
    }
    return handle;
}

bool GfxModule::createPipeline(Core& core, Resolution resolution)
{
    ARES_log(glog, Trace,
             "Creating GfxPipeline (initial resolution: %s)",
             resolution);

    GfxBackend& backend = renderer_->backend();

    pipeline_ = makeRef<GfxPipeline>();

    using Pass = GfxPipeline::Pass;
    using VA = GfxPipeline::Attrib;
    using Ch = ImageFormat::Channel;

    // Load the required shaders for the passes
    // TODO Non-hardcoded shader paths
    Handle<GfxShader> pbrShader = loadShader(core, "Gfx/PBR.arsh");
    Handle<GfxShader> ppShader = loadShader(core, "Gfx/Postprocess.arsh");

    if(!pbrShader || !ppShader)
    {
        return false;
    }

    // #0: PBR pass
    // - Input attribs: matches `Mesh::Vertex`
    // - Shader: PBR.arsh
    // - Output targets:
    //      * Color (RGBA16F, linear): RGB = color and lighting data, A = alpha
    //      * Normals (RGB10A2, linear): RGB = Normals (rescaled from [-1..1] to [0..1])
    //      * RMID (RGBA8, linear): R = roughness, G = metallicity,
    //                              I+D = id of the entity packed as big endian U16
    //      * Velocity (RGB16F, linear): RGB = velocity
    //      * Depth (32F)
    {
        Pass pbrPass;

        pbrPass.attribs[0] = {"position", VA::Type::F32, 3};
        pbrPass.attribs[1] = {"normal", VA::Type::F32, 3};
        pbrPass.attribs[2] = {"tangent", VA::Type::F32, 4};
        pbrPass.attribs[3] = {"texCoord0", VA::Type::F32, 2};
        pbrPass.attribs[4] = {"texCoord1", VA::Type::F32, 2};
        pbrPass.attribs[5] = {"color0", VA::Type::F32, 4};
        pbrPass.nAttribs = 6;

        pbrPass.targets[0] = createPipelineTarget(core, resolution,
                                                  {Ch::F16, Ch::F16, Ch::F16, Ch::F16});
        pbrPass.targets[1] = createPipelineTarget(core, resolution,
                                                  {Ch::UN10, Ch::UN10, Ch::UN10, Ch::UN2});
        pbrPass.targets[2] = createPipelineTarget(core, resolution,
                                                  {Ch::UN8, Ch::UN8, Ch::UN8, Ch::UN8});
        pbrPass.targets[3] = createPipelineTarget(core, resolution,
                                                  {Ch::F16, Ch::F16, Ch::F16});
        pbrPass.targets[4] = createPipelineTarget(core, resolution,
                                                  {Ch::F32Depth});
        pbrPass.nTargets = 5;
        pbrPass.clearTargets = true;

        pbrPass.shader = pbrShader;

        pipeline_->passes.push_back(pbrPass);
    }

    // #1: Postprocess pass
    // - Input attribs: none, generates fullscreen triangles in the vertex shader directly
    // - Shader: Postprocess.arsh
    // - Output targets:
    //      * <Screen>
    {
        Pass ppPass;

        ppPass.nAttribs = 0;

        ppPass.targets[0] = GfxPipeline::Pass::SCREEN_TARGET;
        ppPass.nTargets = 1;
        ppPass.clearTargets = false; // No need to, only rendering fullscreen triangles

        ppPass.shader = ppShader;

        pipeline_->passes.push_back(ppPass);
    }

    resolution_ = resolution;

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

static Mesh gTestMesh; // FIXME!! TEST!!
static Handle<GfxBuffer> gTestMeshVtxBuf, gTestMeshIdxBuf; // FIXME!! TEST!!

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

    {   // FIXME!! TEST!!
        static Mesh::Vertex testVertices[3];
        testVertices[0].position = {-1, -1, 0};
        testVertices[0].color0 = {1, 0, 0, 1};
        testVertices[1].position = {1, -1, 0};
        testVertices[1].color0 = {0, 1, 0, 1};
        testVertices[2].position = {0, 1, 0};
        testVertices[2].color0 = {0, 0, 1, 1};
        gTestMesh.vertices().insert(gTestMesh.vertices().end(),
                                    testVertices, testVertices + 3);
        gTestMeshVtxBuf = backend_->genBuffer({gTestMesh.vertexDataSize(), gTestMesh.vertexData()});

        gTestMesh.indices().push_back(0);
        gTestMesh.indices().push_back(1);
        gTestMesh.indices().push_back(2);
        gTestMeshIdxBuf = backend_->genBuffer({gTestMesh.indexDataSize(), gTestMesh.indexData()});
    }

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


void GfxModule::changeResolution(Core& core, Resolution newResolution)
{
    ARES_log(glog, Trace, "Resolution changed: %s -> %s", resolution_, newResolution);

    // Resize every render target to match the new resolution
    // TODO IMPORTANT - "Render rescale" option for targets
    for(const GfxPipeline::Pass& pass : pipeline_->passes)
    {
        for(unsigned int i = 0; i < pass.nTargets; i ++)
        {
            backend_->resizeTexture(pass.targets[i], newResolution);
        }
    }

    resolution_ = newResolution;
}


void GfxModule::mainUpdate(Core& core)
{
    // Poll events for the current and/or next frame[s]
    // TODO: Move input polling somewhere else so that even if the rendering isof `pipeline_`'s render targets accordingly
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

    {   // FIXME!! TEST!!
        GfxCmd fullscreenPassCmd;
        fullscreenPassCmd.op = GfxCmd::Draw;
        fullscreenPassCmd.passId = 1;
        fullscreenPassCmd.n = 3;
        fullscreenPassCmd.vertexBuffer = {}; // (vertices are generated in the vertex shader)

        const GfxPipeline::Pass& pbrPass = pipeline_->passes[0];
        for(unsigned int i = 0; i < pbrPass.nTargets; i ++)
        {
            fullscreenPassCmd.textures[i] = pbrPass.targets[0];
        }
        fullscreenPassCmd.nTextures = pbrPass.nTargets;

        renderer_->enqueueCmd(fullscreenPassCmd);

        GfxCmd triRenderCmd;
        triRenderCmd.op = GfxCmd::DrawIndexed;
        triRenderCmd.passId = 0;
        triRenderCmd.n = 3;
        triRenderCmd.vertexBuffer = gTestMeshVtxBuf;
        triRenderCmd.indexBuffer = gTestMeshIdxBuf;
        renderer_->enqueueCmd(triRenderCmd);
    }

    auto curResolution = window_->resolution();
    if(curResolution != resolution_)
    {
        changeResolution(core, curResolution);
    }

    renderer_->renderFrame(resolution_);

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
