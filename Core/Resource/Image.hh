#pragma once

#include <stddef.h>
#include <vector>
#include <utility>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "../Visual/Resolution.hh"

namespace Ares
{

/// A 2D raster image.
/// Rows of pixels are stored contiguously in memory, with the top-left pixel
/// being at (0, 0).
template <typename Pixel>
class Image
{
    std::vector<Pixel> data_;
    Resolution resolution_;

public:
    /// Creates a new, uninitialized image (i.e. an image of resolution 0x0).
    constexpr Image()
        : data_(), resolution_{0, 0}
    {
    }

    /// Initializes a new image with the given resolution.
    /// If `data` is not null it will be assumed to be an array of pixel data
    /// of valid size for this image and it will be copied inside it.
    Image(Resolution resolution, Pixel* data=nullptr)
        : resolution_(resolution)
    {
        if(data)
        {
            data_.assign(data, data + (resolution.width * resolution.height));
        }
    }

    Image(const Image<Pixel>& toCopy)
    {
        (void)operator=(toCopy);
    }

    Image& operator=(const Image<Pixel>& toCopy)
    {
        // Copy data
        data_.assign(&toCopy.data_[0], &toCopy.data_[0] + toCopy.data_.size());
        resolution_ = toCopy.resolution_;

        return *this;
    }

    Image(const Image<Pixel>&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    Image& operator=(Image<Pixel>&& toMove)
    {
        // Move data over
        data_ = std::move(toMove.data_);
        resolution_ = toMove.resolution_;

        // Invalidate the moved instance
        toMove.resolution_ = {0, 0};

        return *this;
    }

    /// Destroys pixel data and sets resolution to 0x0.
    ~Image()
    {
        resolution_ = {0, 0};
    }

    /// Returns `true` if the image is valid, i.e. it has non-zero resolution
    /// and has not been moved nor destroyed.
    inline operator bool() const
    {
        return resolution_.width != 0 && resolution_.height != 0;
    }


    /// Resizes the new image to be of the given resolution.
    /// **WARNING**: Does not check if any of the two dimensions is zero!!
    void resize(Resolution newResolution)
    {
        data_.resize(newResolution.width * newResolution.height);
        resolution_ = newResolution;
    }

    /// Returns the current resolution of the image.
    inline Resolution resolution() const
    {
        return resolution_;
    }


    /// Gets/modifies the pixel at the given position.
    /// **WARNING**: Does not perform bounds checking!!
    inline Pixel& at(size_t x, size_t y)
    {
        return data_[y * resolution_.width + x];
    }
    inline Pixel at(size_t x, size_t y) const
    {
        return data_[y * resolution_.width + x];
    }

    /// Returns the pixel data of the image as an array.
    /// See the documentation of `Image` itself.
    const Pixel* data() const
    {
        return &data_[0];
    }

    /// Returns the size in bytes of the image's `data()` array.
    size_t dataSize() const
    {
        return data_.size() * sizeof(Pixel);
    }
};

}
