#pragma once

#include <stddef.h>
#include <string.h>
#include "NumTypes.hh"

namespace Ares
{

/// A string of a fixed `size`.
/// Useful to be used as a key for hash maps.
template <size_t size>
struct KeyString
{
    char str[size];

    KeyString()
    {
        str[0] = '\0';
    }
    KeyString(const char* toCopy)
    {
        strncpy(str, toCopy, size - 1);
        str[size - 1] = '\0';
    }

    inline operator const char*() const
    {
        return str;
    }

    inline bool operator==(const KeyString<size>& other) const
    {
        // Perform a normal, byte-by-byte comparison
        return strncmp(str, other.str, size) == 0;
    }
};

}

namespace std
{

template <size_t size>
struct hash<Ares::KeyString<size>>
{
    size_t operator()(const Ares::KeyString<size>& value) const
    {
        // SDBM hash (public domain).
        // See: http://www.cse.yorku.ca/~oz/hash.html
        Ares::U64 hash = 0;
        for(auto ch = (const unsigned char*)value.str; *ch; ch ++)
        {
            hash = int(*ch) + (hash << 6) + (hash << 16) - hash;
        }
        return hash;
    }
};

}
