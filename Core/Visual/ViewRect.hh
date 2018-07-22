#pragma once

#include <glm/vec2.hpp>
#include "Resolution.hh"

namespace Ares
{

/// A "view rectangle", i.e. a rectangle on a raster screen identifying a viewport.
///
/// The (0, 0) origin of the screen is assumed to be at the top-left of it.
/// 0------> +X
/// |      |
/// |      |
/// v------+
/// +Y
struct ViewRect
{
    /// The top-left corner of the rectangle, relative to the screen's origin.
    glm::tvec2<size_t> topLeft;

    /// The bottom-right corner of the rectangle, relative to the screen's origin.
    glm::tvec2<size_t> bottomRight;


    /// Returns the resolution of the view rectangle.
    /// **WARNING**: Does not work properly if `topLeft.x > bottomRight.x` and/or
    ///              `topLeft.y > bottomRight.y`!!
    inline Resolution resolution() const
    {
        return {bottomRight.x - topLeft.x, bottomRight.y - topLeft.y};
    }


    inline bool operator==(const ViewRect& other) const
    {
        return topLeft == other.topLeft && bottomRight == other.bottomRight;
    }
    inline bool operator!=(const ViewRect& other) const
    {
        return topLeft != other.topLeft || bottomRight != other.bottomRight;
    }
};

inline std::ostream& operator<<(std::ostream& stream, const ViewRect& viewRect)
{
    stream << viewRect.resolution() << '+' << viewRect.topLeft.x << ',' << viewRect.topLeft.y;
    return stream;
}

}
