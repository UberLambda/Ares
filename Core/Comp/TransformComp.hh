#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace Ares
{

/// The transform of a 3D entity.
struct TransformComp
{
    /// The position of the entity in the scene.
    glm::vec3 position{0.0f, 0.0f, 0.0f};

    /// The XYZ Euler rotation of the entity.
    /// Angles are in radians.
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};

    /// A scaling factor for the entity.
    /// (1.0, 1.0, 1.0) is the base scale.
    glm::vec3 scale{1.0f, 1.0f, 1.0f};


    /// Returns a matrix representation of the transform.
    inline glm::mat4 matrix() const
    {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 r = glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z);
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        return t * r * s;
    }
};

}
