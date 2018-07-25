#pragma once

#include <Core/Api.h>
#include <stddef.h>

namespace Ares
{

/// Statistics about memory usage.
/// Not all statistics are implemented for all mem backends. Unimplemented fields'
/// values are always zero.
struct ARES_API MemStats
{
    /// The current number of memory allocations.
    unsigned int nAllocs = 0;

    /// The memory that is currently mapped, in bytes.
    size_t mapped = 0;

    /// All memory that is mapped, in bytes.
    size_t totalMapped = 0;

    /// All memory that is unmapped, in bytes.
    size_t totalUnmapped = 0;
};

/// Updates, then returns current statistics about program-wide (global) memory [de]allocations.
const MemStats& ARES_API memStats();

/// Returns the name of the library used for `malloc()/free()`.
const char* ARES_API memBackendName();

}
