#pragma once

// Global variables and functions defined to use in this mem backend

// TODO This mem backend only works with pthreads for now
#include <pthread.h>

#include <atomic>
#include "../MemStats.hh"

namespace Ares
{

/// Initializes rpmalloc globally for the program and the main thread. Does
/// nothing if already initialized.
///
/// Run by `__attribute__((constructor))` on GCC-like compilers (incl. MinGW)
void rpmallocInit();

/// Halts rpmalloc globally for the program and the current thread.
///
/// Run by `__attribute__((destructor))` on GCC-like compilers (incl. MinGW)
void rpmallocHalt();


/// The global number of mem backend allocations,
/// to be `load()`ed into `gStats` when stats are queried.
extern std::atomic<unsigned int> gNAllocs;

/// The global number of mem backend allocations that had to be leaked.
/// See `Ares::free()`'s implementation.
extern std::atomic<unsigned int> gNAllocsSurelyLeaked;

/// The global mem backend stats.
extern MemStats gStats;

/// `true` if rpmalloc has been initialized for the program.
extern bool gRpmallocInited;

// The `pthread_key` used to automatically finalize rpmalloc when a thread dies.
// TODO This mem backend only works with pthreads for now
extern pthread_key_t gThreadKey;

}



