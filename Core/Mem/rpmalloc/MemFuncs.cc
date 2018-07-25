#include <Core/Mem/MemFuncs.hh>

#include <rpmalloc/rpmalloc.h>
#include "Globals.hh"
#include <stdlib.h> // FIXME

namespace Ares
{

std::atomic<unsigned int> gNAllocsSurelyLeaked{0};  // Defined in "Globals.hh"


// **WARNING**: All these functions rely on "PThreadOverrides.cc" thread/executable
//              hooks doing their job and initializing rpmalloc for each executable
//              and thread in said executable before being called!!

void* malloc(size_t size)
{
    rpmallocInit();

    gNAllocs ++; // (atomic)
    return rpmalloc(size);
}

void* realloc(void* ptr, size_t size)
{
    rpmallocInit();

    return rprealloc(ptr, size);
}

void* calloc(size_t n, size_t size)
{
    rpmallocInit();

    gNAllocs ++; // (atomic)
    return rpcalloc(n, size);
}

void* aligned_alloc(size_t alignment, size_t size)
{
    rpmallocInit();

    gNAllocs ++; // (atomic)
    return rpaligned_alloc(alignment, size);
}

void free(void* ptr)
{
    if(ptr)
    {
        gNAllocs --; // (atomic)

        if(gRpmallocInited && rpmalloc_is_thread_initialized())
        {
            rpfree(ptr);
        }
        else
        {
            // If we reached here we were trying to get rpmalloc to free a pointer
            // even if it was not initialized globally / for this thread. This
            // could happen when trying to `free()` a pointer after rpmalloc
            // finalization (could be in a `__attribute__((destructor))`) or if
            // trying to free a pointer that was not allocated by rpmalloc in
            // the first place.

            // If this happens, **do nothing but record a memory leak.**
            // It is better to leak the pointer - expecially because we most
            // probably are at the end of the program's life cycle, when the OS
            // will regain all resources - than potentially to crash the whole
            // program by attempting to free the pointer anyways.

            gNAllocsSurelyLeaked ++; // (atomic)
        }
    }
}

}
