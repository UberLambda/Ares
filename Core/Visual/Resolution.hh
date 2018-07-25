#pragma once

#include <stddef.h>
#include <ostream>
#include <Core/Api.h>

namespace Ares
{

/// The resolution of a raster image.
struct ARES_API Resolution
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

inline std::ostream& ARES_API operator<<(std::ostream& stream, const Resolution& resolution)
{
    stream << resolution.width << 'x' << resolution.height;
    return stream;
}

}
