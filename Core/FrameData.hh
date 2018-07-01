#pragma once

#include "Event/EventMatrix.hh"

namespace Ares
{

/// Engine data stored on a frame-by-frame basis.
///
/// This data is doublebuffered; two `FrameData`s ("current" and "past")
/// are alternated in a ping-pong fashion.
/// **Rendering/simulation/AI... is always one frame behind!**
/// This heavily simplifies concurrency, since modules can run at any
/// time/in any order during a frame - displaying/receiving events from a
/// consistent, readonly "past" frame data, and pushing new events at will on the
/// read/write "current" frame data.
struct FrameData
{
    /// The event matrix for events that happened this frame.
    ///
    /// Events are to be pushed (potentially concurrently) to the "current" frame
    /// data, to be iterated and processed next frame by reading the now-"past" frame data.
    /// Events in the past frame data are cleared at the end of the current frame.
    EventMatrix events;


    /// Clears the frame data. This is done to prepare it for the next update cycle,
    /// when it will be recycled as the new "current" frame data.
    void clear()
    {
        events.clearAllQueues();
    }
};

}
