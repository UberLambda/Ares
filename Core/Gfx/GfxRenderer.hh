#pragma once

#include <stddef.h>
#include <unordered_map>
#include <vector>
#include <concurrentqueue.h>
#include <Core/Base/MapTree.hh>
#include <Core/Base/Handle.hh>
#include <Core/Base/Ref.hh>
#include <Core/Visual/Resolution.hh>
#include <Core/Gfx/GfxResources.hh>
#include <Core/Gfx/GfxCmd.hh>
#include <Core/Gfx/GfxPipeline.hh>
#include <Core/Gfx/GfxBackend.hh>

namespace Ares
{

class GfxRenderer
{
private:
    Ref<GfxBackend> backend_;
    Ref<GfxPipeline> pipeline_;

    moodycamel::ConcurrentQueue<GfxCmd> cmdQueue_; ///< The global command queue.
    moodycamel::ConsumerToken cmdQueueTok_; ///< A queue consumer token to be
                                            ///< used by `renderFrame()` only

    Resolution frameResolution_; ///< This frame's rendering resolution.
    std::vector<GfxCmd> frameCmds_; ///< All of this frame's rendering commands, unsorted.
    std::vector<GfxCmdIndex> frameCmdsOrder_; ///< The sorted indices to `frameCmds_`.

    /// A tree of all the combinations of textures (from `textures[0]` to possibly
    /// `textures[MAX_TEXTURES]`) used in all of this frame commands'. Each
    /// combination of textures in each command is put in as a key in the tree
    /// before the commands themselves are run; when iterating it afterwards the
    /// index of the iterator can be used as part of the sort key for commands to
    /// minimize the amount of times the textures have to be rebound (see how
    /// `MapTree` stores nodes internally to understand!).
    MapTree<U32, size_t> frameTextures_;


    /// Generates sorted indices into `frameCmdsOrder_` for the first `n` commands
    /// currently in `frameCmds_`.
    void orderFrameCmds(size_t n);

public:
    /// Initializes a new renderer given a handle to the backend it will use.
    /// Call `init()` to actually be able to start executing rendering commands!
    GfxRenderer(Ref<GfxBackend> backend);

    /// Destroys the renderer.
    ~GfxRenderer();


    /// [Re]initializes the renderer, setting it to use the given rendering
    /// pipeline for the commands it will run. Returns an error message on error.
    ErrString init(Ref<GfxPipeline> pipeline);


    /// Returns a reference to the pipeline currently used by the renderer.
    inline Ref<GfxPipeline> pipeline() const
    {
        return pipeline_;
    }

    /// Returns a reference to the backend used by the renderer.
    /// Identical to `operator->()`.
    inline GfxBackend& backend()
    {
        return *backend_;
    }

    /// Alias of `backend()` for convenience.
    inline GfxBackend* operator->()
    {
        return backend_.get();
    }


    /// Pushes `n` `GfxCmd`s into the rendering command queue, to be executed
    /// later by `renderFrame()`.
    /// Threadsafe and lockless.
    template <typename CmdIterator>
    inline void enqueueCmds(CmdIterator begin, size_t n)
    {
        cmdQueue_.enqueue_bulk(begin, n);
    }

    /// Pushes `n` `GfxCmd`s into the rendering command queue, to be executed
    /// later by `renderFrame()`.
    /// Threadsafe and lockless.
    inline void enqueueCmd(const GfxCmd& cmd)
    {
        cmdQueue_.enqueue(cmd);
    }

    /// Copies all commands currently in the rendering command queue to an internal
    /// buffer, sorts them by pass and to minimize resource (textures, buffers, ...)
    /// rebinds as much as possible, then has the backend execute them.
    /// If resolution changed from the latest `renderFrame()` call also invokes
    /// `backend().changeResolution()` before running the commands.
    void renderFrame(Resolution resolution);
};

}
