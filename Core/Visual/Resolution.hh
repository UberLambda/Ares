#pragma once

#include <stddef.h>

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
};

}
