#pragma once

#include <stddef.h>
#include <unordered_map>
#include <vector>
#include <concurrentqueue.h>
#include "../Base/MapTree.hh"
#include "../Base/Handle.hh"
#include "../Base/Ref.hh"
#include "../Visual/Resolution.hh"
#include "GfxResources.hh"
#include "GfxCmd.hh"
#include "GfxPipeline.hh"
#include "GfxBackend.hh"

namespace Ares
{

class GfxRenderer
{
private:
    Ref<GfxBackend> backend_;
    Ref<GfxPipeline> pipeline_;

    moodycamel::ConcurrentQueue<GfxCmd> cmdQueue_;
    moodycamel::ConsumerToken cmdQueueTok_; // A queue consumer token to be
                                            // used by `renderFrame()` only

    Resolution frameResolution_;
    MapTree<U32, size_t> frameMaterials_;
    std::vector<GfxCmd> frameCmds_;
    std::vector<GfxCmdIndex> frameCmdsOrder_;

    /// Generates sorted indices into `frameCmdsOrder_` for the first `n` commands
    /// currently in `frameCmds_`.
    void orderFrameCmds(size_t n);

public:
    GfxRenderer(Ref<GfxBackend> backend);
    ~GfxRenderer();

    /// [Re]initializes the backend with the given pipeline.
    ErrString init(Ref<GfxPipeline> pipeline);

    inline Ref<GfxPipeline> pipeline() const
    {
        return pipeline_;
    }

    inline GfxBackend* operator->()
    {
        return backend_.get();
    }

    void renderFrame(Resolution resolution);
};

}
