// Conditional source file compilation based on allocator backend
#include <Ares/BuildConfig.h>
#ifdef ARES_HAS_RPMALLOC
//  rpmalloc mem backend
#   include "rpmalloc/MemStats.cc"
#   include "rpmalloc/MemFuncs.cc"
#   include "rpmalloc/Hooks.cc"
#else
//  stdlib mem backend
#   include "stdlib/MemStats.cc"
#   include "stdlib/MemFuncs.cc"
#endif
