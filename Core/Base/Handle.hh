#pragma once

#include <stddef.h>
#include <Core/Api.h>
#include <Core/Base/NumTypes.hh>

namespace Ares
{

/// A numeric handle for a `T` resource.
/// Handles with id 0 are "null handles", i.e. pointing to no resource.
template <typename T>
struct ARES_API Handle
{
    U32 id;


    constexpr Handle()
        : id(0)
    {
    }
    explicit constexpr Handle(U32 id)
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
class ARES_API hash<Ares::Handle<T>>
{
public:
    inline size_t operator()(const Ares::Handle<T>& value) const
    {
        return size_t(value.id);
    }
};

}
