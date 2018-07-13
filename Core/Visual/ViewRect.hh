#pragma once

#include <glm/vec2.hpp>
#include "Resolution.hh"

namespace Ares
{

/// A "view rectangle", i.e. a rectangle on a raster screen identifying a viewport.
/// The (0, 0) origin of the screen is assumed to be at the bottom-left of it.
struct ViewRect
{
    /// The bottom-left corner of the rectangle, relative to the screen's origin.
    glm::uvec2 bottomLeft;

    /// The top-right corner of the rectangle, relative to the screen's origin.
    glm::uvec2 topRight;


    /// Returns the resolution of the view rectangle.
    /// **WARNING**: Does not work properly if `bottomLeft.x > topRight.x` and/or
    ///              `bottomRight.y > topRight.y`!!
    inline Resolution resolution() const
    {
        glm::uvec2 diff = topRight - bottomLeft;
        return {size_t(diff.x), size_t(diff.y)};
    }


    inline bool operator==(const ViewRect& other) const
    {
        return bottomLeft == other.bottomLeft && topRight == other.topRight;
    }
    inline bool operator!=(const ViewRect& other) const
    {
        return bottomLeft != other.bottomLeft || topRight != other.topRight;
    }
};

inline std::ostream& operator<<(std::ostream& stream, const ViewRect& viewRect)
{
    stream << viewRect.resolution() << '+' << viewRect.bottomLeft.x << ',' << viewRect.bottomLeft.y;
    return stream;
}

}
