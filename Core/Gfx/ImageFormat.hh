#pragma once

#include <ostream>
#include "../Base/NumTypes.hh"

namespace Ares
{

/// The format of each pixel in an image. This is composed by 4 `ChannelFormats`
/// (msB to lsB: r, g, b, a). See `ImageFormat::isValid()` for extra restrictions.
struct ImageFormat
{
    /// The format of an `ImageFormat`'s channel.
    enum class Channel : U8
    {
        None = 0, ///< Channel disabled.

        I8, ///< 8-bit signed integer.
        I10, ///< 10-bit signed integer.
        I16, ///< 16-bit signed integer.
        I32, ///< 32-bit signed integer.

        U2, ///< 2-bit unsigned integer.
        U8, ///< 8-bit unsigned integer.
        U10, ///< 10-bit unsigned integer.
        U16, ///< 16-bit unsigned integer.
        U32, ///< 32-bit unsigned integer.

        UN2, ///< 2-bit unsigned integer, normalized to 0..1 float
        UN8, ///< 8-bit unsigned integer, normalized to 0..1 float
        UN10, ///< 10-bit unsigned integer. normalized to 0..1 float
        UN16, ///< 16-bit unsigned integer, normalized to 0..1 float

        F16, ///< 16-bit floating point number.
        F32, ///< 32-bit floating point number.

        F32Depth, ///< Special format: 32-bit floating point depth.
                  ///  Use only a single `F32Depth` `r` channel in a depth texture.

        Max = F32, ///< The maximum value the `Channel` enum may take.
    };


    union
    {
        struct { Channel r, g, b, a; };
        Channel channels[4];
    };

    /// Initializes an image format with all channels set to `None`.
    constexpr ImageFormat()
        : ImageFormat(Channel::None, Channel::None, Channel::None, Channel::None)
    {
    }

    /// Initializes an image format from 1 to 4 channel formats.
    constexpr ImageFormat(Channel r,
                          Channel g=Channel::None, Channel b=Channel::None, Channel a=Channel::None)
        : r(r), g(g), b(b), a(a)
    {
    }

    /// Initializes an image format given its `U32` packed representation.
    /// See `operator U32()`.
    constexpr ImageFormat(U32 packed)
        : r(Channel((packed & 0xFF000000U) >> 24U)),
          g(Channel((packed & 0x00FF0000U) >> 16U)),
          b(Channel((packed & 0x0000FF00U) >> 8U)),
          a(Channel((packed & 0x000000FFU)))
    {
    }


    /// Returns `true` if the image format is valid.
    /// For the image format to be valid, either [r], [r, g], [r, g, b] or
    /// [r, g, b, a] must be set to a non-`None` valid channel format; no other combination
    /// of `None` and non-`None` channel formats is acceptable.
    inline bool isValid() const
    {
        for(unsigned int i = 1; i < 4; i ++)
        {
            if((channels[i - 1] == Channel::None && channels[i] != Channel::None)
                || channels[i - 1] > Channel::Max || channels[i] > Channel::Max)
            {
                return false;
            }
        }
        return true;
    }

    /// Returns the number of channels set, i.e. whose format is not `None`.
    inline unsigned int nChannelsSet() const
    {
        unsigned int n = 0;
        for(unsigned int i = 0; i < 4; i ++)
        {
            n += unsigned(channels[i] != Channel::None);
        }
        return n;
    }

    /// Returns `true` if this format is for a depth texture.
    inline bool isDepth() const
    {
        return channels[0] == Channel::F32Depth;
    }

    /// Packs the type of the four channels of the image format into an `U32`;
    /// msB to lsB: r, g, b, a.
    inline operator U32() const
    {
        return (U32(r) << 24U) | (U32(g) << 16U) | (U32(b) << 8U) | U32(a);
    }
};

}
