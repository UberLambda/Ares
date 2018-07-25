#include "GfxModule.hh"

#include <utility>
#include <unordered_map>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Core/Core.hh>
#include <Core/Debug/Log.hh>
#include <Core/Data/ResourceLoader.hh>
#include <Core/Visual/Window.hh>
#include <Core/Resource/Mesh.hh>
#include <Core/Scene/Scene.hh>
#include <Core/Scene/SceneIterator.hh>
#include <Core/Comp/TransformComp.hh>
#include <Core/Comp/MeshComp.hh>
#include <Core/Gfx/GfxRenderer.hh>
#include <Core/Gfx/GfxBackend.hh>

// OpenGL 3.3 core backend
#include <flextGL.h>
#include <Core/Gfx/GL33/Backend.hh>

namespace Ares
{

struct GfxModule::Data
{
    struct PbrUniforms
    {
        glm::mat4 camViewProj;

    } pbrUniforms;
    Handle<GfxBuffer> pbrUniformsBuffer;

    struct MeshBatch
    {
        size_t count;
        std::vector<glm::mat4> modelMatrices;
        Handle<GfxBuffer> vertexBuffer = {}, indexBuffer = {}, instanceBuffer = {};
        size_t vertexBufferSize = 0, indexBufferSize = 0, instanceBufferSize = 0;
    };
    std::unordered_map<Ref<Mesh>, MeshBatch> meshMap_;
};


GfxModule::GfxModule()
    : window_(nullptr), resolution_{0, 0}, renderer_(nullptr), data_(nullptr)
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
    // - Input attribs:
    //      * [0..5]: Matching `Mesh::Vertex`'s ones
    //      * [6..9] : Model matrix for the drawn instance, split into 4 vec4s
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
        pbrPass.attribs[6] = {"modelMatrixR0", VA::Type::F32, 4, 1};
        pbrPass.attribs[7] = {"modelMatrixR1", VA::Type::F32, 4, 1};
        pbrPass.attribs[8] = {"modelMatrixR2", VA::Type::F32, 4, 1};
        pbrPass.attribs[9] = {"modelMatrixR3", VA::Type::F32, 4, 1};
        pbrPass.nAttribs = 10;

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

        GfxBufferDesc uniformsBufferDesc;
        uniformsBufferDesc.size = sizeof(Data::PbrUniforms);
        uniformsBufferDesc.data = nullptr;
        uniformsBufferDesc.usage = GfxUsage::Streaming;
        data_->pbrUniformsBuffer = renderer_->backend().genBuffer(uniformsBufferDesc);
        pbrPass.uniformBuffer = data_->pbrUniformsBuffer;

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

        ppPass.depthTestEnabled = false; // Not needed here, only drawing fullscreen triangles

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

    data_ = new Data();

    Resolution initialResolution = window_->resolution();

    bool allOk = initGL(core)
                 && createRenderer(core)
                 && createPipeline(core, initialResolution)
                 && initPipelineAndRenderer(core);
    if(!allOk)
    {
        return false;
    }

    return true;
}

void GfxModule::halt(Core& core)
{
    // Destoy data
    delete data_; data_ = nullptr;

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
            backend_->resizeTexture(pass.targets[i], newResolution, 1);
        }
    }

    resolution_ = newResolution;
}


void GfxModule::genSceneCmds(Core& core)
{
    for(auto batchIt = data_->meshMap_.begin(); batchIt != data_->meshMap_.end(); batchIt ++)
    {
        batchIt->second.count = 0;
        batchIt->second.modelMatrices.clear();
    }

    Scene* scene = core.g().scene;
    for(auto it = scene->begin(); it != scene->end(); it ++)
    {
        auto transformComp = it->comp<TransformComp>();
        auto meshComp = it->comp<MeshComp>();
        if(transformComp && meshComp)
        {
            // Add this mesh's model matrix to the appropriate drawing batch
            Data::MeshBatch& meshBatch = data_->meshMap_[meshComp->mesh];
            meshBatch.count ++;
            meshBatch.modelMatrices.push_back(transformComp->matrix());
        }
    }

    auto& backend = renderer_->backend();

    for(auto batchIt = data_->meshMap_.begin(); batchIt != data_->meshMap_.end();
        batchIt ++)
    {
        if(batchIt->second.count == 0)
        {
            // FIXME IMPORTANT Mark batches whose `count == 0` (i.e. for meshes
            //       that haven't been drawn even once this frame) for cleanup.
            //       Do not delete them immediately here; delete them only when
            //       GPU memory is getting scarce, or after a set number of frames
            //       in which the mesh hasn't been rendered even once, or after
            //       some other trigger happens. Deleting them immediately is much
            //       more likely in resulting in having to reupload them a couple
            //       of frames down the road because they are needed again!
            continue;
        }


        const Mesh& mesh = *batchIt->first;
        Data::MeshBatch& batch = batchIt->second;

        if(!batch.vertexBuffer)
        {
            // First time we draw this Mesh, create its vertex buffer
            GfxBufferDesc vertexBufferDesc;
            batch.vertexBufferSize = vertexBufferDesc.size = mesh.vertexDataSize();
            vertexBufferDesc.data = mesh.vertexData();
            vertexBufferDesc.usage = GfxUsage::Dynamic; // FIXME: Static for StaticMeshComp, dynamic for DynamicMeshComp
            batch.vertexBuffer = backend.genBuffer(vertexBufferDesc);
        }
        else if(batch.indexBufferSize != mesh.indexDataSize())
        {
            // Mesh's vertex data was resized, update its vertex buffer
            batch.vertexBufferSize = mesh.vertexDataSize();
            backend.resizeBuffer(batch.vertexBuffer, batch.vertexBufferSize);
            backend.editBuffer(batch.vertexBuffer,
                               0, batch.vertexBufferSize,
                               mesh.vertexData());
        }
        // FIXME IMPORTANT Also check if **the contents** of the vertex data in the
        //       Mesh have changed since last frame; if they did, update `vertexBuffer`'s
        //       contents accordingly


        if(mesh.indexDataSize() > 0)
        {
            if(!batch.indexBuffer)
            {
                // First time we draw this Mesh, create its index buffer
                GfxBufferDesc indexBufferDesc;
                batch.indexBufferSize = indexBufferDesc.size = mesh.indexDataSize();
                indexBufferDesc.data = mesh.indexData();
                indexBufferDesc.usage = GfxUsage::Dynamic; // FIXME: Static for StaticMeshComp, dynamic for DynamicMeshComp
                batch.indexBuffer = backend.genBuffer(indexBufferDesc);
            }
            else if(batch.indexBufferSize != mesh.indexDataSize())
            {
                // Mesh's index data was resized, update its index buffer
                batch.indexBufferSize = mesh.indexDataSize();
                backend.resizeBuffer(batch.indexBuffer, batch.indexBufferSize);
                backend.editBuffer(batch.indexBuffer,
                                   0, batch.indexBufferSize,
                                   mesh.indexData());
            }
            // FIXME IMPORTANT Also check if **the contents** of the index data in the
            //       Mesh have changed since last frame; if they did, update `indexBuffer`'s
            //       contents accordingly
        }

        if(!batch.instanceBuffer)
        {
            // First time we draw this Mesh batch, create its instance buffer
            GfxBufferDesc instanceBufferDesc;
            batch.instanceBufferSize = instanceBufferDesc.size
                                     = batch.modelMatrices.size() * sizeof(glm::mat4);
            instanceBufferDesc.data = batch.modelMatrices.data();
            instanceBufferDesc.usage = GfxUsage::Streaming;
            batch.instanceBuffer = renderer_->backend().genBuffer(instanceBufferDesc);
        }
        else
        {
            // Update the instance buffer, growing it if necessary
            // If the buffer is actually bigger there is no harm (except some more
            // memory consumption) in leaving it that size and only filling it partially
            size_t newInstanceBufferSize = batch.modelMatrices.size() * sizeof(glm::mat4);
            if(newInstanceBufferSize > batch.instanceBufferSize)
            {
                backend.resizeBuffer(batch.instanceBuffer, newInstanceBufferSize);
                batch.instanceBufferSize = newInstanceBufferSize;
            }

            backend.editBuffer(batch.instanceBuffer,
                               0, newInstanceBufferSize,
                               batch.modelMatrices.data());
        }


        // Generate the GfxCmd to render the batch
        // FIXME IMPORTANT Bind the correct textures gathered from Material here!
        GfxCmd batchRenderCmd;
        batchRenderCmd.passId = 0;
        if(mesh.indices().size() > 0)
        {
            batchRenderCmd.op = GfxCmd::DrawIndexedInstanced;
            batchRenderCmd.n = mesh.indices().size();
        }
        else
        {
            batchRenderCmd.op = GfxCmd::DrawInstanced;
            batchRenderCmd.n = mesh.vertices().size();
        }
        batchRenderCmd.first = 0;
        batchRenderCmd.nInstances = batch.count;
        batchRenderCmd.vertexBuffer = batch.vertexBuffer;
        batchRenderCmd.indexBuffer = batch.indexBuffer;
        batchRenderCmd.instanceBuffer = batch.instanceBuffer;

        renderer_->enqueueCmd(batchRenderCmd);
    }
}

void GfxModule::mainUpdate(Core& core)
{
    // NOTE: Window events (incl. resizing!) are polled by `InputModule`

    // Execute rendering commands calculate for the previous frame
    // NOTE: *THIS IS ALWAYS ONE FRAME BEHIND!*
    //       Rendering commands for a frame are generated by worker threads by
    //       `updateTask()`, but `mainUpdate()` - that actually executes them -
    //       is run before `updateTask()`. This means that the rendering commands
    //       run will always be the ones generated for the previous frame,
    //       introducing a one-frame rendering lag
    window_->beginFrame();

    auto curResolution = window_->resolution();
    if(curResolution != resolution_)
    {
        changeResolution(core, curResolution);
    }

    // FIXME TEST!! Test camera viewProjection matrix
    {
        glm::mat4 camProj = glm::perspectiveFov(glm::half_pi<float>(), // 90°
                                                float(resolution_.width), float(resolution_.height),
                                                0.1f, 1000.0f);
        glm::mat4 camView = glm::lookAt(glm::vec3(5.0f), glm::vec3(0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
        data_->pbrUniforms.camViewProj = camProj * camView;
    }

    // Update uniform data for passes
    renderer_->backend().editBuffer(data_->pbrUniformsBuffer,
                                    0, sizeof(Data::PbrUniforms),
                                    &data_->pbrUniforms);

    // Generate commands for rendering Scene data in pass 0
    genSceneCmds(core);

    // Generate the final command for outputting the final image for pass 1
    GfxCmd ppDrawCmd;
    ppDrawCmd.op = GfxCmd::Draw;
    ppDrawCmd.passId = 1;
    ppDrawCmd.n = 3; // One fullscreen triangle
    ppDrawCmd.vertexBuffer = {}; // No vertex buffers, positions generated in vertex shader
    const auto& pbrPass = pipeline_->passes[0];

    for(unsigned int i = 0; i < pbrPass.nTargets; i ++)
    {
        ppDrawCmd.textures[i] = pbrPass.targets[i];
    }
    ppDrawCmd.nTextures = pbrPass.nTargets;
    renderer_->enqueueCmd(ppDrawCmd);

    // Sort and execute rendering commands and swap buffers
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
