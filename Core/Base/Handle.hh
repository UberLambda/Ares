#pragma once

#include <stddef.h>
#include "../Base/NumTypes.hh"

namespace Ares
{

/// A numeric handle for a `T` resource.
/// Handles with id 0 are "null handles", i.e. pointing to no resource.
template <typename T>
struct Handle
{
    U32 id;


    Handle()
        : id(0)
    {
    }
    explicit Handle(U32 id)
        : id(id)
    {
    }

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

namespace std
{

template <typename T>
class hash<Ares::Handle<T>>
{
public:
    inline size_t operator()(const Ares::Handle<T>& value) const
    {
        return size_t(value.id);
    }
};

}
