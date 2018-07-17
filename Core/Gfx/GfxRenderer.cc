#include "GfxRenderer.hh"

namespace Ares
{

GfxRenderer::GfxRenderer()
    : cmdQueueTok_(cmdQueue_)
{
}

GfxRenderer::~GfxRenderer()
{
    // `~GfxBackend()`, that will be run when the last `Ref<GfxBackend>` to the
    // backend dies, will Destroy all leftover resources
}

ErrString GfxRenderer::init(Ref<GfxBackend> backend)
{
    if(backend)
    {
        backend_ = backend;
        auto err = backend_->init();
        return err;
    }
    else
    {
        return "Can't init null GfxBackend!";
    }
}


void GfxRenderer::renderFrame()
{
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
    //      `program`+`textures` in `GfxCmd`

    // TODO PERFORMANCE Multithreaded (Task-based) index generation + sorting?

    // Rebuild the material tree for this frame.
    // Priority used for minimizing rebinds:
    // - Shader program
    // - Textures [0..n]
    // - vertex & indexBuffer - FIXME: Should not be in the *material* tree!
    // After the tree is built, iterating into the tree will automatically give
    // the optimal binding order and, by accessing `iterator.index()`, also a
    // sorting key for materials
    frameMaterials_.clear();
    for(size_t i = 0; i < n; i ++)
    {
        const GfxCmd& cmd = frameCmds_[i];

        auto it = frameMaterials_.at(cmd.shader);
        for(unsigned int i = 0; i < GfxCmd::MAX_TEXTURES; i ++)
        {
            it = it.at(cmd.textures[i]);
        }
        it = it.at(cmd.vertexBuffer, cmd.indexBuffer); // FIXME See above
    }

    // Build the sorting indices
    // Priority used for sorting:
    // - Rendering pass (hi to lo)
    // - Material index
    static_assert(sizeof(GfxCmd::pass) == 1,
                  "Sort key generation code expects GfxCmd::pass to be an U8");

    for(size_t i = 0; i < n; i ++)
    {
        const GfxCmd& cmd = frameCmds_[i];
        GfxCmdIndex& cmdIndex = frameCmdsOrder_[i];

        auto matIt = frameMaterials_.at(cmd.shader);
        for(unsigned int i = 0; i < GfxCmd::MAX_TEXTURES; i ++)
        {
            matIt = matIt.at(cmd.textures[i]);
        }
        matIt = matIt.at(cmd.vertexBuffer, cmd.indexBuffer); // FIXME See above
        U32 matIndex = U32(matIt.index());

        cmdIndex.index = i;
        cmdIndex.key = U64(cmd.pass) << 56 | U64(matIndex);
    }

    // Order the command indices
    std::sort(frameCmdsOrder_.begin(), frameCmdsOrder_.begin() + n);
}

}
