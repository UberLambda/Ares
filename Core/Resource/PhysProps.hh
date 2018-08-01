#pragma once

#include <Core/Api.h>

namespace Ares
{

/// The properties of a physics material.
struct ARES_API PhysProps
{
    /// The friction coefficient of the rigid body, [0..1].
    float friction = 0.0f;

    /// The rolling friction coefficient of the rigid body, [0..1].
    float rollingFriction = 0.0f;

    /// The restitution coefficient of the rigid body, [0..1].
    float restitution = 0.5f;

    /// The linear damping coefficient of the rigid body, [0..1].
    float linearDamping = 0.0f;

    /// The angular damping coefficient of the rigid body, [0..1].
    float angularDamping = 0.0f;
};

}
