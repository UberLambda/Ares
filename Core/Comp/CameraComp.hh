#pragma once

#include <Core/Api.h>
#include <Core/Base/NumTypes.hh>
#include <Core/Gfx/PerspectiveCamera.hh>


namespace Ares
{

/// A camera attached to an entity.
struct ARES_API CameraComp
{
    enum Type
    {
        Perspective = 0, ///< `PerspectiveCamera`
    } type;

    PerspectiveCamera perspective; ///< Used if `type == Perspective`.


    /// The priority of the camera. The camera with **lowest** priority in the scene
    /// will be used for rendering.
    ///
    /// **WARNING**: If there are multiple cameras with the same priority any of
    /// them could be used per-frame!
    U32 priority = 0;
};

}
