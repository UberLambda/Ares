#pragma once

#include "../Base/NumTypes.hh"
#include "../Base/KeyString.hh"

namespace Ares
{

/// An handle to a particular resource in a `ResourcePack`.
struct ResourceHandle
{
    /// The type of resource ("texture", "audio"...).
    KeyString<8> type{};

    /// The id of this particular resource, unique for each type.
    U32 id = -1;


    /// Returns `true` if the given handle is valid (valid type string + valid id).
    inline operator bool() const
    {
        return type.hash() != 0 && id != U32(-1);
    }

    inline bool operator==(const ResourceHandle& other) const
    {
        return type == other.type && id == other.id;
    }
};

}
