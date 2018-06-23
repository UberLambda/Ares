#pragma once

#include <unordered_map>
#include "ResourceHandle.hh"

namespace Ares
{

/// A pack of resources, mapping each resource's filepath to the handle the resource
/// would have when loaded.
class ResourcePack
{
public:
    /// The type of the inner `resource filepath -> resource handle` map.
    // FIXME With this system, `n > 1` filepaths can point to the same resource handle -
    //       leaving resource pack loaders to load from a random filepath of the
    //       `n`! Use a bimap (unique key <=> value pairs in both directons) instead!
    using Map = std::unordered_map<std::string, ResourceHandle>;

private:
    Map map_;

    ResourcePack(const ResourcePack& toCopy) = delete;
    ResourcePack& operator=(const ResourcePack& toCopy) = delete;

    ResourcePack(ResourcePack&& toMove) = delete;
    ResourcePack&& operator=(ResourcePack&& toMove) = delete;

public:
    ResourcePack() = default;
    ~ResourcePack() = default;

    /// Gets a mutable reference to the handle the resource with the given path
    /// would have; will insert the path in the pack if not already present.
    inline ResourceHandle& operator[](const std::string& resPath)
    {
        return map_[resPath];
    }

    /// Gets a copy of the handle the resource with the given path would have;
    /// returns an invalid handle if no such filepath is present.
    inline ResourceHandle operator[](const std::string& resPath) const
    {
        auto it = map_.find(resPath);
        if(it != map_.end())
        {
            return it->second;
        }
        else
        {
            return {};
        }
    }


    /// Copies all `filepath -> handle` pairs from `other` into self, replacing
    /// handles that differ in `other` and leaving handles present in self but
    /// not `other` unmodified.
    void merge(const ResourcePack& other)
    {
        for(auto& it : other)
        {
            map_[it.first] = it.second;
        }
    }


    inline Map::const_iterator begin() const
    {
        return map_.cbegin();
    }

    inline Map::const_iterator end() const
    {
        return map_.cend();
    }

    inline Map::const_iterator cbegin() const
    {
        return begin();
    }

    inline Map::const_iterator cend() const
    {
        return end();
    }
};

}
