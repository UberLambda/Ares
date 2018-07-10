#pragma once

#include <glm/vec3.hpp>

namespace Ares
{

/// The transform of a 3D entity.
struct TransformComp
{
    /// The position of the entity in the scene.
    glm::vec3 position;

    /// The XYZ Euler rotation of the entity.
    /// Angles are in radians.
    glm::vec3 rotation;

    /// A scaling factor for the entity.
    /// (1.0, 1.0, 1.0) is the base scale.
    glm::vec3 scale;
};

}
