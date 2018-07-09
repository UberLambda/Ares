#pragma once

#include <stddef.h>
#include <ostream>

namespace Ares
{

/// The resolution of a raster image.
struct Resolution
{
    size_t width = 0, height = 0;


    inline bool operator==(const Resolution& other) const
    {
        return width == other.width && height == other.height;
    }

    inline bool operator!=(const Resolution& other) const
    {
        return width != other.width || height != other.height;
    }
};

inline std::ostream& operator<<(std::ostream& stream, const Resolution& resolution)
{
    stream << resolution.width << 'x' << resolution.height;
    return stream;
}

}
