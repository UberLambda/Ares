#include "GfxRenderer.hh"

namespace Ares
{

GfxRenderer::GfxRenderer(Ref<GfxBackend> backend)
    : backend_(backend), cmdQueueTok_(cmdQueue_),
      frameResolution_{0, 0} // (set to 0x0 to make sure that `changeVideoMode` is run at startup)
{
}

GfxRenderer::~GfxRenderer()
{
    // `~GfxBackend()`, that will be run when the last `Ref<GfxBackend>` to the
    // backend dies, will Destroy all leftover resources
}

ErrString GfxRenderer::init(Ref<GfxPipeline> pipeline)
{
    if(!backend_)
    {
        return "GfxBackend is null";
    }

    auto err = backend_->init(pipeline);
    pipeline_ = pipeline;
    return err;
}


void GfxRenderer::renderFrame(Resolution resolution)
{
    // Check if resolution changed; if so, run backend callback
    if(frameResolution_ != resolution)
    {
        backend_->changeResolution(resolution);
        frameResolution_ = resolution;
    }

    // Copy all commands that we can find from the global command queue to this
    // frame's command queue
    size_t nCmds = cmdQueue_.size_approx();
    if(nCmds > frameCmds_.size())
    {
        // TODO Grow `frameCmds_`, `frameCmdsOrder_` exponentially every time
        //      instead of just matching the new number of commands in the queue?
        frameCmds_.resize(nCmds);
        frameCmdsOrder_.resize(nCmds);
    }
    nCmds = cmdQueue_.try_dequeue_bulk(cmdQueueTok_, &frameCmds_[0], nCmds);

    // Calculate the order the commands will be run in
    orderFrameCmds(nCmds);

    // Run the ordered commands
    backend_->runCmds(&frameCmds_[0], &frameCmdsOrder_[0], nCmds);
}

void GfxRenderer::orderFrameCmds(size_t n)
{
    // TODO PERFORMANCE IMPORTANT Rebuild the material tree only once per material
    //      addition/deletion, not for every command for every frame!!
    //      This could be done by storing a `Handle<GfxMaterial>` instead of
    //      `textures` in `GfxCmd`

    // TODO PERFORMANCE Multithreaded (Task-based) index generation + sorting?

    // Rebuild the material tree for this frame.
    // Draw calls are grouped together by the textures they use.
    // After the tree is built, iterating into the tree will automatically give
    // the optimal binding order and, by accessing `iterator.index()`, also a
    // sorting key for materials.

    // There are **no** attempts to minimize vertex/index buffer rebinds!! This
    // is because if the vertex/index buffers are the same you should not use 2+
    // different `Draw[Indexed]` calls, but a single `Draw[Indexed]Instanced` call
    // instead!
    // TODO Could try to merge ("batch") `Draw[Indexed]` commands with the same
    //      buffers to `Draw[Indexed]Instanced` calls here!

    frameMaterials_.clear();
    for(size_t i = 0; i < n; i ++)
    {
        const GfxCmd& cmd = frameCmds_[i];

        // TODO Extract this to its own function (see below)
        auto it = frameMaterials_.begin();
        for(unsigned int i = 0;
            i < GfxCmd::MAX_TEXTURES && cmd.textures[i] != 0;
            i ++)
        {
            it = it.at(cmd.textures[i]);
        }
    }

    // Build the sorting indices
    // Priority used for sorting:
    // - Rendering pass (lo to hi)
    // - Material index
    static_assert(sizeof(GfxCmd::passId) == 1,
                  "Sort key generation code expects GfxCmd::pass to be an U8");

    for(size_t i = 0; i < n; i ++)
    {
        const GfxCmd& cmd = frameCmds_[i];
        GfxCmdIndex& cmdIndex = frameCmdsOrder_[i];

        // TODO Extract this to its own function (see above)
        auto matIt = frameMaterials_.begin();
        for(unsigned int i = 0;
            i < GfxCmd::MAX_TEXTURES && cmd.textures[i] != 0;
            i ++)
        {
            matIt = matIt.at(cmd.textures[i]);
        }
        U32 matIndex = U32(matIt.index());

        // Key layout: see docs
        cmdIndex.index = i;
        cmdIndex.key = U64(cmd.passId) << 56 | U64(matIndex);
    }

    // Order the command indices by key
    std::sort(frameCmdsOrder_.begin(), frameCmdsOrder_.begin() + n);
}

}
