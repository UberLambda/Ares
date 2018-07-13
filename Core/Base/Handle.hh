#pragma once

#include "../Base/NumTypes.hh"

namespace Ares
{

/// A numeric handle for a `T` resource.
/// Handles with id 0 are "null handles", i.e. pointing to no resource.
template <typename T>
struct Handle
{
    U32 id;

    inline operator U32() const
    {
        return id;
    }

    inline bool operator==(const Handle& other) const
    {
        return id == other.id;
    }
    inline bool operator!=(const Handle& other) const
    {
        return id != other.id;
    }
};

}
