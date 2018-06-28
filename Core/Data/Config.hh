#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include "../Base/NumTypes.hh"

namespace Ares
{

/// A key in a config.
using ConfigKey = std::string;

/// A value in a config.
struct ConfigValue
{
    enum Type
    {
        I64,
        F64,
        String,
        Boolean,
    } type = I64; ///< The type of value.

    struct
    {
        std::string string; // (should not allocate memory if empty or small)
        union
        {
            Ares::I64 i64 = 0;
            Ares::F64 f64;
            bool boolean;
        };
    } value; ///< The value itself. **WARNING** Only one field is valid, depending on `type`!


    bool operator==(const ConfigValue&& other) const
    {
        if(type != other.type)
        {
            return false;
        }

        switch(type)
        {
        case I64:
            return value.i64 == other.value.i64;

        case F64:
            return value.f64 == other.value.f64;

        case String:
            return value.string == other.value.string;

        default: // Boolean
            return value.boolean == other.value.boolean;
        }
    }
};

/// A key/value config.
class Config
{
public:
    // The internal `ConfigKey -> ConfigValue` type used.
    using Map = std::unordered_map<ConfigKey, ConfigValue>;

private:
    Map map_;

public:
    Config() = default;
    ~Config() = default;

    /// Gets a copy of the value associated to the given key in the config, or a
    /// copy of `fallback` if the key was not found.
    inline ConfigValue get(const ConfigKey& key, ConfigValue fallback=ConfigValue()) const
    {
        auto it = map_.find(key);
        if(it != map_.end())
        {
            return it->second;
        }
        else
        {
            return fallback;
        }
    }

    /// Sets the value associated to the given key in the config to a copy of `value`,
    /// inserting the key if was not previously present.
    inline void set(const ConfigKey& key, const ConfigValue& value)
    {
        map_[key] = value;
    }

    /// Erases any value associated to the given key in the config.
    inline void erase(const ConfigKey& key)
    {
        map_.erase(key);
    }

    /// Returns `true` if a value associated to the given key is present in the
    /// config.
    inline bool has(const ConfigKey& key) const
    {
        return map_.find(key) != map_.end();
    }


    inline Map::const_iterator begin() const
    {
        return map_.cbegin();
    }
    inline Map::const_iterator cbegin() const
    {
        return begin();
    }

    inline Map::const_iterator end() const
    {
        return map_.cend();
    }
    inline Map::const_iterator cend() const
    {
        return end();
    }
};

}
