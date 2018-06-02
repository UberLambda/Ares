#pragma once

#include <stddef.h>
#include <string.h>
#include "NumTypes.hh"

namespace Ares
{

/// A hashed string of a fixed `size`.
/// Useful to be used as a key for hash maps.
template <size_t size>
class KeyString
{
    char str_[size] = {'\0'};
    U64 hash_ = 0;

    KeyString(KeyString<size>&& toMove) = delete;
    KeyString& operator=(KeyString<size>&& toMove) = delete;

public:
    /// Creates a new, empty key string.
    KeyString()
        : hash_(0)
    {
        str_[0] = '\0';
    }

    /// Creates a new keystring by copying at most `size - 1` characters from `str`.
    constexpr KeyString(const char* str)
    {
        // Copy as many characters as possible
        size_t len = 0;
        for(len = 0; len < (size - 1) && str[len] != '\0'; len ++)
        {
            str_[len] = str[len];
        }
        str_[len] = '\0';

        // Hash string with SDBM hash (public domain)
        // See: http://www.cse.yorku.ca/~oz/hash.html
        // FIXME Should iterate `unsigned char`s, not `char`s - but should work anyways
        //       constexpr constructors do not allow `reinterpret_cast`s...
        hash_ = 0;
        for(auto ch = str_; *ch; ch ++)
        {
            hash_ = int(*ch) + (hash_ << 6) + (hash_ << 16) - hash_;
        }
    }

    constexpr KeyString(const KeyString<size>& toCopy)
    {
        (void)operator=(toCopy);
    }

    constexpr KeyString& operator=(const KeyString<size>& toCopy)
    {
        strncpy(str_, toCopy.str_, size);
        hash_ = toCopy.hash_;
        return *this;
    }

    inline operator const char*() const
    {
        return str_;
    }

    inline U64 hash() const
    {
        return hash_;
    }

    /// **IMPORTANT**: Other is passed by value so that temporary strings can be
    ///                as hashmap keys; this fails if it's passed by reference
    ///                since a reference to a temporary will produce unexpected
    ///                results
    inline bool operator==(const KeyString<size> other) const
    {
        // Perform a normal, byte-by-byte comparison
        return strncmp(str_, other.str_, size) == 0;
    }
};

}

namespace std
{

template <size_t size>
struct hash<Ares::KeyString<size>>
{
    inline size_t operator()(const Ares::KeyString<size>& value) const
    {
        return value.hash();
    }
};

}
