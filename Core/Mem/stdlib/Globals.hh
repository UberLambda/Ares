#pragma once

// Global variables defined to use in this mem backend

#include <atomic>
#include "../MemStats.hh"

namespace Ares
{

/// The global number of mem backend allocations,
/// to be `load()`ed into `gStats` when stats are queried.
extern std::atomic<unsigned int> gNAllocs;

/// The global mem backend stats.
extern MemStats gStats;

}


