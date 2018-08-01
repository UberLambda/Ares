#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <Core/Api.h>

namespace Ares
{

/// A 3D collision shape.
struct ARES_API CollisionShape
{
    enum Type
    {
        Box, ///< A box of size `size`.
        Sphere, ///< A sphere of diameter `min(size.x, size.y, size.z)`.
        Cylinder, ///< A cylinder of base diameter `min(size.x, size.z)` and height `size.y`.
        Capsule, ///< A capsule of base diameter `min(size.x, size.z)` and total height `size.y`.
        Cone, ///< A cone of base diameter `min(size.x, size.z)` and height `size.y`.
        Plane, ///< A plane with normal `normalDist.xyz` and constant `normalDist.w`.
               ///  **WARNING** Can only be use with `Static` `RigidBody`es.

        ConvexHull, ///< The smallest convex shape enclosing all `data[i].xyz` point.
        Spheres, ///< A set of spheres, each centered at `data[i].xyz` and with radius `data[i].w`.

    } type = Spheres; ///< The type of shape used.

    glm::vec3 size; ///< A size, in meters x meters x meters.
    glm::vec4 normalDist; ///< A XYZ normal + W constant vector for `Plane`s.

    std::vector<glm::vec4> data; ///< See `Type::ConvexHull`, `Type::Spheres`.
};

}
