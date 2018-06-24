#pragma once

#include "../Base/Serializer.hh"
#include "../Base/CoreSerializers.hh"

namespace Ares
{

template <typename T>
struct Serializer<ResourceRef<T>>
{
    inline static bool serialize(const ResourceRef<T>& value, std::ostream& stream)
    {
        // FIXME Should also serialize the `ResourceLoader` in some way?
        return Serializer<ResourceHandle<T>>::serialize(value.handle_, stream);
    }

    inline static bool deserialize(ResourceRef<T>& value, std::istream& stream)
    {
        // FIXME Should also serialize the `ResourceLoader` in some way?
        return Serializer<ResourceHandle<T>>::deserialize(value.handle_, stream);
    }
};

}
