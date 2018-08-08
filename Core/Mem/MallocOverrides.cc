// Replaces libc/stdlib implementations of standard C allocation functions with
// Ares ones

// Base on "rpmalloc/malloc.c" (public domain), to which obsolete functions have
// been removed

// Works only on platforms where precedence is given to these redefined symbols
// over the system's (i.e. libc) ones. On other platforms, the program may still
// decide to use standard malloc/free/... for C code, in which case most C (but
// not C++, see `NewOverrides.cc`) memory allocations will be untracked and remain
// managed by the system's allocator.

#include <stddef.h>

#include <Core/Api.h>
#include <Core/Mem/MemFuncs.hh>


// See "rpmalloc/rpmalloc.h" (public domain)
#ifdef _MSC_VER
# define ARES_RESTRICT __declspec(restrict)
# define ARES_CDECL __cdecl
#else
# define ARES_RESTRICT
# define ARES_CDECL
#endif

extern "C"
{
// **THESE SYMBOLS WILL HAVE PRECEDENCE OVER libc's !!**

extern ARES_API ARES_RESTRICT void* ARES_CDECL malloc(size_t size)
// C standard library
{
    return Ares::malloc(size);
}

extern ARES_API ARES_RESTRICT void* ARES_CDECL calloc(size_t n, size_t size)
// C standard library
{
    return Ares::calloc(n, size);
}

extern ARES_API void* ARES_CDECL realloc(void* ptr, size_t size)
// C standard library
{
    return Ares::realloc(ptr, size);
}

extern ARES_API ARES_RESTRICT void* ARES_CDECL aligned_alloc(size_t alignment, size_t size)
// C (C11) standard library
{
    return Ares::aligned_alloc(alignment, size);
}

extern ARES_API void ARES_CDECL free(void* ptr)
// C standard library
{
    Ares::free(ptr);
}


extern ARES_API int ARES_CDECL posix_memalign(void** memptr, size_t alignment, size_t size)
// POSIX C extension
{
    *memptr = Ares::aligned_alloc(alignment, size);
    return (*memptr) == nullptr; // (0 on success, 1 on error)
}


extern ARES_API void* ARES_CDECL reallocf(void* ptr, size_t size)
// BSD C extension
{
    return Ares::realloc(ptr, size);
}

extern ARES_API void* ARES_CDECL reallocarray(void* ptr, size_t count, size_t size)
// BSD C extension
{
    // FIXME Actually check for arguments' validity! (see `reallocarray()`'s manual)
    return Ares::realloc(ptr, count * size);
}

// memalign: obsolete POSIX C extension
// valloc: obsolete POSIX C extension
// pvalloc: obsolete POSIX C extension
// cfree: obsolete POSIX C extension
// malloc_usable_size: GNU C extension, obsoleted on Android

} // extern "C"


// TODO IMPLEMENT For Mac Os X declaring these static functions is probably
//                not enough, use MAC_INTERPOSE?
