#pragma once

#include <Core/Api.h>
#include <stddef.h>

#ifdef _MSC_VER
//  #undef MSVC memory allocation macros
#   pragma warning (disable : 4100)
#   undef malloc
#   undef realloc
#   undef calloc
//  NOTE: MSVC does not have `aligned_alloc`
#   undef free
#endif

namespace Ares
{

/// Ares' drop-in replacement for `malloc()`.
/// Implementation depends on the Mem backend in use.
void* ARES_API malloc(size_t size);

/// Ares' drop-in replacement for `realloc()`.
/// Implementation depends on the Mem backend in use.
void* ARES_API realloc(void* ptr, size_t size);

/// Ares' drop-in replacement for `calloc()`.
/// Implementation depends on the Mem backend in use.
void* ARES_API calloc(size_t n, size_t size);

/// Ares' drop-in replacement for `aligned_malloc()` (C11).
// Implementation depends on the Mem backend in use.
void* ARES_API aligned_alloc(size_t alignment, size_t size);

/// Ares' drop-in replacement for `free()`.
/// Implementation depends on the Mem backend in use.
void ARES_API free(void* ptr);

}
