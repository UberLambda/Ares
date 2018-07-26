#pragma once

#include <Core/Api.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Ares
{

/// A perspective camera located at the origin of the coordinate system defined
/// by a `TransformComp`.
struct ARES_API PerspectiveCamera
{
    /// The horizontal field of view of the camera, in radians.
    float fov = glm::radians(90.0f);

    /// The camera's near plane.
    float zNear = 0.1f;

    /// The camera's far plane.
    float zFar = 1000.0f;


    /// Calculates a projection matrix for the camera given its viewport's aspect
    /// ratio.
    inline glm::mat4 projectionMatrix(float aspectRatio) const
    {
        return glm::perspectiveFov(fov, aspectRatio, 1.0f, zNear, zFar);
    }
};

}
