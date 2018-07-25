#pragma once

#include <utility>
#include <unordered_map>
#include <Core/Api.h>
#include <Core/Base/KeyString.hh>

namespace Ares
{

/// A virtual axis, i.e. a floating point value that changes according to input.
using Axis = float;

/// A name to identify a particular input `Axis`.
using AxisName = KeyString<16>;

/// A mapping of `Axis` names to their values.
class ARES_API AxisMap
{
    std::unordered_map<AxisName, Axis> map_;

    AxisMap(const AxisMap& toCopy) = delete;
    AxisMap& operator=(const AxisMap& toCopy) = delete;

public:
    /// Initalizes an empty axis map.
    AxisMap() = default;

    AxisMap(AxisMap&& toMove)
    {
        (void)operator=(std::move(toMove));
    }
    AxisMap& operator=(AxisMap&& toMove)
    {
        // Move data over
        map_ = std::move(toMove.map_);

        return *this;
    }

    ~AxisMap() = default;


    /// Gets the value of axis in the map with the given name.
    /// If the axis is not found in the map, returns `0`.
    inline const Axis operator[](const AxisName& name) const
    {
        auto it = map_.find(name);
        if(it != map_.end())
        {
            return it->second;
        }
        else
        {
            // Undefined axis
            return 0;
        }
    }

    /// Gets a mutable reference to the value of axis in the map with the given
    /// name. If the axis is not found in the map, a new one with the given name
    /// is created, set to `0` and a reference to it is returned.
    inline Axis& operator[](const AxisName& name)
    {
        return map_[name];
    }

    /// Returns `true` if the map contains an axis with the given name or `false`
    /// otherwise.
    inline bool has(const AxisName& name) const
    {
        return map_.find(name) != map_.end();
    }
};

}
