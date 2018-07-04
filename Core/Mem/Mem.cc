// Conditional source file compilation based on allocator backend
#include <Ares/BuildConfig.h>
#ifdef ARES_HAS_RPMALLOC
//  rpmalloc mem backend
#   include "rpmalloc/MemStats.cc"
#   include "rpmalloc/MemFuncs.cc"
#   ifdef ARES_HAS_PTHREADS
//      Replaces `pthread_create` with a rpmalloc-aware one and adds pre-main/post-main
//      hooks to each shared library/executable using rpmalloc
#       include "rpmalloc/PThreadOverrides.cc"
#   else
//      Need some kind of hook to init/deinit rpmalloc per-thread, but don't know
//      how without pthreads
#       error "defined(ARES_HAS_RPMALLOC) but !defined(ARES_HAS_PTHREADS)"
#   endif
#else
//  stdlib mem backend
#   include "stdlib/MemStats.cc"
#   include "stdlib/MemFuncs.cc"
#endif
