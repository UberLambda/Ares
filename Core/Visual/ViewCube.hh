#pragma once

#include <glm/vec3.hpp>
#include <Core/Api.h>
#include <Core/Visual/Resolution.hh>

namespace Ares
{

/// A "view cube", i.e. a cube on a 3D raster screen identifying a 3D viewport.
///
/// The (0, 0, 0) origin is assumed to be at the top-front-left of it.
///+Z-------+
/// |\      |\
/// | \     | \
/// +==0-------> +X
///  \ |     \ |
///   \|      \|
///    v-------+
///    +Y
struct ARES_API ViewCube
{
    /// The top-front-left corner of the rectangle, relative to the screen's origin.
    glm::tvec3<size_t> topFrontLeft;

    /// The bottom-back-right corner of the rectangle, relative to the screen's origin.
    glm::tvec3<size_t> bottomBackRight;


    /// Returns the XY resolution of the view cube.
    /// **WARNING**: Does not work properly if `topFrontLeft.x > bottomBackRight.x` and/or
    ///              `topFrontLeft.y > bottomBackRight.y`!!
    inline Resolution xyResolution() const
    {
        return {bottomBackRight.x - topFrontLeft.x,
                bottomBackRight.y - topFrontLeft.y};
    }

    /// Returns the Z depth of the view cube, i.e. the difference between its
    /// back and front depth level.
    /// **WARNING**: Does not work properly if `topFrontLeft.z > bottomBackRight.z`!
    inline size_t zDepth() const
    {
        return bottomBackRight.z - topFrontLeft.z;
    }


    inline bool operator==(const ViewCube& other) const
    {
        return topFrontLeft == other.topFrontLeft
               && bottomBackRight == other.bottomBackRight;
    }
    inline bool operator!=(const ViewCube& other) const
    {
        return topFrontLeft != other.topFrontLeft
               || bottomBackRight != other.bottomBackRight;
    }
};

inline std::ostream& ARES_API operator<<(std::ostream& stream, const ViewCube& viewCube)
{
    const glm::uvec3& tfl = viewCube.topFrontLeft;
    stream << viewCube.xyResolution(); stream << 'x'; stream << viewCube.zDepth();
    stream << '+';
    stream << tfl.x; stream << ','; stream << tfl.y; stream << ','; stream << tfl.z;
    return stream;
}

}
