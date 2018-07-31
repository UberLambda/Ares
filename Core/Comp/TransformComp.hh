#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Core/Api.h>

namespace Ares
{

/// The transform of a 3D entity.
struct ARES_API TransformComp
{
    /// The position of the entity in the scene.
    glm::vec3 position{0.0f, 0.0f, 0.0f};

    /// The rotation of the entity.
    glm::quat rotation{glm::vec3(0.0f, 0.0f, 0.0f)};

    /// A scaling factor for the entity.
    /// (1.0, 1.0, 1.0) is the base scale.
    glm::vec3 scale{1.0f, 1.0f, 1.0f};


    /// Returns a matrix representation of the transform.
    inline glm::mat4 matrix() const
    {
        // NOTE that `rotation` is normalized before converting it to a matrix;
        //      the resulting matrix would be broken if a non-normalized `rotation`
        //      was supplied otherwise!
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 r = glm::toMat4(glm::normalize(rotation));
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        return t * r * s;
    }
};

}
