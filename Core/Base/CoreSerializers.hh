#pragma once

#include <string>
#include <type_traits>
#include <Ares/BuildConfig.h>
#include "NumTypes.hh"
#include "LinTypes.hh"

// === Serializers for core types, that are mostly easy to serialize ===========

namespace Ares
{

/// Implements a `Serializer<T>` for the trivial-to-serialize type `T`.
/// Trivial-to-serialize types can just be cast from/to character arrays and
/// read/written directly (altough they are always read/written in big endian form).
///
/// Note that no security checks are performed on the [de]serialized values except
/// that the correct number of bytes could be read/written!
#ifdef ARES_PLATFORM_IS_BIG_ENDIAN
#   define ARES_implTrivialSerializer(T) \
        template <> \
        struct Serializer<T> \
        { \
            static_assert(sizeof(char) == 1, \
                          "sizeof(char) != 1, trivial serializers won't work!"); \
         \
            inline static bool serialize(const T& value, std::ostream& stream) \
            { \
                const char* valueMem = reinterpret_cast<const char*>(&value); \
                /* Big endian: write the value out directly */ \
                stream.write(valueMem, sizeof(T)); \
                return bool(stream); /* (`false` if could not write all bytes) */ \
            } \
         \
            inline static bool deserialize(T& value, std::istream& stream) \
            { \
                char* valueMem = reinterpret_cast<char*>(&value); \
                /* Big endian: just read the big endian value */ \
                stream.read(valueMem, sizeof(T)); \
                return bool(stream); /* (`false` if could not read all bytes) */ \
            } \
        };

#else
//  Platform is little endian
#   define ARES_implTrivialSerializer(T) \
        template <> \
        struct Serializer<T> \
        { \
            static_assert(sizeof(char) == 1, \
                          "sizeof(char) != 1, trivial serializers won't work!"); \
         \
            inline static bool serialize(const T& value, std::ostream& stream) \
            { \
                const char* valueMem = reinterpret_cast<const char*>(&value); \
                /* Little endian: swap endianness, then write the value */ \
                char invMem[sizeof(T)]; \
                swapEndianness<sizeof(T)>(invMem, valueMem); \
                stream.write(invMem, sizeof(T)); \
                return bool(stream); /* (`false` if could not write all bytes) */ \
            } \
         \
            inline static bool deserialize(T& value, std::istream& stream) \
            { \
                char* valueMem = reinterpret_cast<char*>(&value); \
                /* Little endian: get the little endian value, convert to and write big endian */ \
                char invMem[sizeof(T)]; \
                stream.read(invMem, sizeof(T)); \
                swapEndianness<sizeof(T)>(valueMem, invMem); \
                return bool(stream); /* (`false` if could not read all bytes) */ \
            } \
        };

#endif


// ===== Numeric and char types ================================================
// NOTE: `swapEndianness` is accelerated for `sizeof(T)` in [1, 2, 4, 8]; that
//       means that serialization of {I,U}{8,16,32,64} should be quite fast once
//       the compiler optimizes the generated code!

ARES_implTrivialSerializer(char)
ARES_implTrivialSerializer(I8) // *NOT* equivalent to char!
ARES_implTrivialSerializer(U8) // = unsigned char
ARES_implTrivialSerializer(I16) // = signed short
ARES_implTrivialSerializer(U16) // = unsigned short
ARES_implTrivialSerializer(I32) // = int, [long]
ARES_implTrivialSerializer(U32) // = unsigned int, [unsigned long]
ARES_implTrivialSerializer(I64) // = signed long long
ARES_implTrivialSerializer(U64) // = unsigned long long
ARES_implTrivialSerializer(float)
ARES_implTrivialSerializer(double)

// ===== bool ==================================================================
template <>
struct Serializer<bool>
{
    /// The character to output if the bool is `true`.
    static constexpr const char TRUE_CH = 't';

    /// The character to output if the bool is `false`.
    static constexpr const char FALSE_CH = 'f';


    inline static bool serialize(const bool& value, std::ostream& stream)
    {
        char ch = value ? TRUE_CH : FALSE_CH;
        stream.write(&ch, 1);
        return bool(stream); // (`false` on error)
    }

    inline static bool deserialize(bool& value, std::istream& stream)
    {
        char ch = 'X'; // (nor `TRUE_CH` or `FALSE_CH`)
        stream.read(&ch, 1);
        if(!stream)
        {
            // Failed reading bool char
            return false;
        }

        if(ch == TRUE_CH)
        {
            value = true;
            return true;
        }
        else if(ch == FALSE_CH)
        {
            value = false;
            return true;
        }
        else
        {
            // Unrecognized bool char
            return false;
        }
    }
};

// ===== Strings ===============================================================
template <>
struct Serializer<std::string>
{
    inline static bool serialize(const std::string& value, std::ostream& stream)
    {
        // Serialize string length
        U32 len = value.length(); // FIXME Could [hypotetically...] overflow!
        Serializer<U32>::serialize(len, stream);

        // Write string including null terminator
        stream.write(value.c_str(), len + 1);

        return bool(stream); // (`false` on error)
    }

    inline static bool deserialize(std::string& value, std::istream& stream)
    {
        // Deserialize string length
        U32 len = -1;
        if(!Serializer<U32>::deserialize(len, stream))
        {
            // Failed to deserialize string length
            return false;
        }

        // Read string
        value.resize(len);
        stream.read(&value[0], len);
        if(!stream)
        {
            // Failed to read string
            return false;
        }

        // Check if null terminator is present
        char lastCh = 'X';
        stream.read(&lastCh, 1);
        if(!stream || lastCh != '\0')
        {
            // Corrupt (non-null-terminated) string
            return false;
        }

        return true; // All good
    }
};


// ===== Vec{2,3,4} ============================================================

template <>
struct Serializer<Vec2>
{
    inline static bool serialize(const Vec2& value, std::ostream& stream)
    {
        Serializer<F32>::serialize(value.x, stream);
        Serializer<F32>::serialize(value.y, stream);
        return bool(stream); // (`false` on error)
    }

    inline static bool deserialize(Vec2& value, std::istream& stream)
    {
        Serializer<F32>::deserialize(value.x, stream);
        Serializer<F32>::deserialize(value.y, stream);
        return bool(stream); // (`false` on error)
    }
};

template <>
struct Serializer<Vec3>
{
    inline static bool serialize(const Vec3& value, std::ostream& stream)
    {
        Serializer<F32>::serialize(value.x, stream);
        Serializer<F32>::serialize(value.y, stream);
        Serializer<F32>::serialize(value.z, stream);
        return bool(stream); // (`false` on error)
    }

    inline static bool deserialize(Vec3& value, std::istream& stream)
    {
        Serializer<F32>::deserialize(value.x, stream);
        Serializer<F32>::deserialize(value.y, stream);
        Serializer<F32>::deserialize(value.z, stream);
        return bool(stream); // (`false` on error)
    }
};

template <>
struct Serializer<Vec4>
{
    inline static bool serialize(const Vec4& value, std::ostream& stream)
    {
        Serializer<F32>::serialize(value.x, stream);
        Serializer<F32>::serialize(value.y, stream);
        Serializer<F32>::serialize(value.z, stream);
        Serializer<F32>::serialize(value.w, stream);
        return bool(stream); // (`false` on error)
    }

    inline static bool deserialize(Vec4& value, std::istream& stream)
    {
        Serializer<F32>::deserialize(value.x, stream);
        Serializer<F32>::deserialize(value.y, stream);
        Serializer<F32>::deserialize(value.z, stream);
        Serializer<F32>::deserialize(value.w, stream);
        return bool(stream); // (`false` on error)
    }
};


}
