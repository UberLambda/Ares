#include <Core/Mem/MemFuncs.hh>

// Replaces libc++/stdlib implementations of standard C++ allocation operators with
// Ares ones

// Base on "rpmalloc/new.cc" (public domain)

// By the C++ standard if these operators are implemented somewhere in a linked-in
// translation unit the whole program will use them over the default implementation.
// Hence this operators will redefine the program-wide behaviour of these operators.

#include <new>


// NOTE: Technically, some of these operators should throw `std::bad_alloc` on error,
//       but we don't use exceptions here

ARES_API extern void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return Ares::malloc(size);
}

ARES_API extern void* operator new(size_t size)
{
    return Ares::malloc(size);
}

ARES_API extern void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return Ares::malloc(size);
}

ARES_API extern void* operator new[](size_t size)
{
    return Ares::malloc(size);
}


ARES_API extern void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    Ares::free(ptr);
}

ARES_API extern void operator delete(void* ptr) noexcept
{
    Ares::free(ptr);
}

ARES_API extern void operator delete(void* ptr, size_t size) noexcept // (C++14)
{
    Ares::free(ptr);
}

ARES_API extern void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    Ares::free(ptr);
}

ARES_API extern void operator delete[](void* ptr) noexcept
{
    Ares::free(ptr);
}

ARES_API extern void operator delete[](void* ptr, size_t size) noexcept // (C++14)
{
    // Behaves identically to unsized `free()`
    Ares::free(ptr);
}
