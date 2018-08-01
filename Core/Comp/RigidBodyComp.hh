#pragma once

#include <Core/Api.h>
#include <Core/Base/Ref.hh>
#include <Core/Resource/CollisionShape.hh>
#include <Core/Resource/PhysProps.hh>

namespace Ares
{

/// Marks an entity to be a physics rigid body.
/// The mesh's transform will be updated according to how the rigid body moves.
struct ARES_API RigidBodyComp
{
    enum Type
    {
        Static, ///< Static: The rigid body does not move, but it can collide with other bodies.
        Dynamic, ///< Dynamic: The rigid body is fully affected by the simulation and collisions.
        Kinematic, ///< Kinematic: The movement of the rigid body is controlled by logic external
                   ///  to the simulation.
    } type = Dynamic; ///< The type of rigid body.

    /// The rigid body's collision shape.
    Ref<CollisionShape> collisionShape;

    /// The mass of the rigid body in kilograms.
    /// Ignored if the body is static.
    float mass = 1.0f;

    /// The physical properties of the rigid body.
    PhysProps props;
};

}
