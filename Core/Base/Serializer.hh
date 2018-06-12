#pragma once

#include <istream>
#include <ostream>
#include <Ares/BuildConfig.h>

namespace Ares
{

/// The interface of a [de]serializer for `T`s; to be implemented for each `T`
/// to be [de]serialized.
///
/// Serialized values must be in big endian.
template <typename T>
struct Serializer
{
    /// Attempts to serialize a `T` to a stream; returns `false` on error.
    static bool serialize(const T& value, std::ostream& stream);

    /// Attempts to deserialize a `T` from a stream; returns `false` on error.
    static bool deserialize(T& value, std::istream& stream);
};

}
