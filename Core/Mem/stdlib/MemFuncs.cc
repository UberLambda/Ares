#include "../MemFuncs.hh"

#include <assert.h>
#include "../../Base/Platform.h"
#include "Globals.hh"

namespace Ares
{

// Need to get and store pointers to standard C allocation functions to call them
// since we're overwriting those function names with `Ares::{malloc,realloc,calloc,aligned_alloc,free}`
// in "MallocOverrides.cc"!

using PFN_malloc = void*(*)(size_t size);
using PFN_realloc = void*(*)(void* ptr, size_t size);
using PFN_calloc = void*(*)(size_t n, size_t size);
using PFN_aligned_alloc = void*(*)(size_t alignment, size_t size);
using PFN_free = void(*)(void* ptr);

static struct
{
    bool populated = false;

    PFN_malloc malloc = nullptr;
    PFN_realloc realloc = nullptr;
    PFN_calloc calloc = nullptr;
    PFN_aligned_alloc aligned_alloc = nullptr;
    PFN_free free = nullptr;

} gStdFuncs;

}


#if defined(ARES_PLATFORM_IS_POSIX) || defined(ARES_PLATFORM_IS_MAC)
//  POSIX/Mac OS: use `RTLD_NEXT` to get function pointers to standard alloc functions
#   include <dlfcn.h>

namespace Ares
{

static void populateGStdFuncs()
{
    if(!gStdFuncs.populated)
    {
        // FIXME IMPORTANT ==> http://optumsoft.com/dangers-of-using-dlsym-with-rtld_next/ <==

        // HACK Load calloc first - see HACK in `Ares::calloc()` implementation below
        gStdFuncs.calloc = (PFN_calloc)dlsym(RTLD_NEXT, "calloc");
        assert(gStdFuncs.calloc && "Failed to load calloc");

        gStdFuncs.malloc = (PFN_malloc)dlsym(RTLD_NEXT, "malloc");
        assert(gStdFuncs.malloc && "Failed to load malloc");

        gStdFuncs.realloc = (PFN_realloc)dlsym(RTLD_NEXT, "realloc");
        assert(gStdFuncs.realloc && "Failed to load realloc");

        gStdFuncs.aligned_alloc = (PFN_aligned_alloc)dlsym(RTLD_NEXT, "aligned_alloc");
        assert(gStdFuncs.aligned_alloc && "Failed to load aligned_alloc");

        gStdFuncs.free = (PFN_free)dlsym(RTLD_NEXT, "free");
        assert(gStdFuncs.free && "Failed to load free");


        gStdFuncs.populated = true;
    }
}

}

#elif defined(ARES_PLATFORM_IS_WINDOWS)
//  MSVC defines malloc, calloc, ... as macros in the non-standard header <malloc.h>
//  These have been `#undef`'d in `MemFuncs.hh`.
//  HACK We can use the Win32 API to load the "msvcrt*.dll" file the program is
//       using for the standard library (even MinGW internally uses the old,
//       Visual Studio 6-era "msvcrt.dll") and then get function pointers to
//      `malloc()/free()/...` from there
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>

namespace Ares
{
//  MSVC and MinGW do not have C11's `aligned_alloc`, but do have an
//  `_aligned_malloc` function which is the same but with arguments swapped
using PFN__aligned_malloc = void*(*)(size_t size, size_t alignment);
static PFN__aligned_malloc gWinAlignedMallocFunc = nullptr;

static void* winAlignedAlloc(size_t alignment, size_t size)
{
    return gWinAlignedMallocFunc(size, alignment);
}

static void populateGStdFuncs()
{
    if(!gStdFuncs.populated)
    {
        // FIXME "msvcrt.dll" works for MinGW, but MSVC-compiled programs could
        //       use a different .dll for the C standard library!
        HMODULE msvcrt = GetModuleHandle("msvcrt");
        assert(msvcrt && "Failed to get a handle to msvcrt.dll");


        gStdFuncs.malloc = (PFN_malloc)GetProcAddress(msvcrt, "malloc");
        assert(gStdFuncs.malloc && "Failed to load malloc");

        gStdFuncs.realloc = (PFN_realloc)GetProcAddress(msvcrt, "realloc");
        assert(gStdFuncs.realloc && "Failed to load realloc");

        gStdFuncs.calloc = (PFN_calloc)GetProcAddress(msvcrt, "calloc");
        assert(gStdFuncs.calloc && "Failed to load calloc");

        // See `gWinAlignedMallocFunc` definition above
        gWinAlignedMallocFunc = (PFN__aligned_malloc)GetProcAddress(msvcrt, "_aligned_malloc");
        assert(gWinAlignedMallocFunc && "Failed to load _aligned_malloc");
        gStdFuncs.aligned_alloc = winAlignedAlloc; // (NOTE: != gWinAlignedMallocFunc!!)

        gStdFuncs.free = (PFN_free)GetProcAddress(msvcrt, "free");
        assert(gStdFuncs.free && "Failed to load free");


        gStdFuncs.populated = true;
    }
}

}

#endif


namespace Ares
{

void* malloc(size_t size)
{
    populateGStdFuncs();

    gNAllocs ++; // (atomic)
    return gStdFuncs.malloc(size);
}

void* realloc(void* ptr, size_t size)
{
    populateGStdFuncs();

    return gStdFuncs.realloc(ptr, size);
}

void* calloc(size_t n, size_t size)
{
    // HACK See: https://blog.bigpixel.ro/interposing-calloc-on-linux
    //      When loading `gStdFuncs` pointers with ld, `dlsym()` internally calls
    //      calloc, which is this function, which then will try to load function
    //      pointers again by call `dlsym`... in an infinite loop!
    //      Returning a null pointer from `calloc` invocations made by `dlsym`
    //      (as if the allocation failed for some reason) does not seem to impact
    //      `dlsym`, that can then actually load `calloc()` and continue its job!
    if(!gStdFuncs.populated)
    {
        return nullptr;
    }

    populateGStdFuncs();

    gNAllocs ++; // (atomic)
    return gStdFuncs.calloc(n, size);
}

void* aligned_alloc(size_t alignment, size_t size)
{
    populateGStdFuncs();

    gNAllocs ++; // (atomic)
    return gStdFuncs.aligned_alloc(alignment, size);
}

void free(void* ptr)
{
    populateGStdFuncs();

    if(ptr)
    {
        gNAllocs --; // (atomic)
        gStdFuncs.free(ptr);
    }
}

}
