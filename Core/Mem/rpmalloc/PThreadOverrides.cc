// Global override of `pthread_create` to make threads rpmalloc-aware
// See "rpmalloc/malloc.c" (public domain)

#ifdef _MSC_VER
#   error "pthread_create override only supported on GCC/clang/MinGW!"
//  Not a problem really, considering that MSVC would not use winpthreads anyways...
#endif

#include <stdlib.h>
#include <assert.h>

#include "Globals.hh"
#include "../../Base/Platform.h"


// Attribute of a function that is to be run when an executable/shared library is loaded
// **WARNING**: place it inbetween the return type and the function name!
#define ARES_INIT_HOOK __attribute__((constructor))

// Attribute of a function that is to be run when an executable/shared library is unloaded
// **WARNING**: place it inbetween the return type and the function name!
#define ARES_HALT_HOOK __attribute__((destructor))


namespace Ares
{

    bool gRpmallocInited = false; // Declared in "Globals.hh"

    pthread_key_t gThreadKey; // Declared in "Globals.hh"


    /// Data required to start a new thread.
    extern "C" struct ThreadStartData
    {
        void*(*func)(void*); ///< The function to be run by the thread.
        void* arg; ///< The argument passed to `func`.
    };

    /// A function that inits rpmalloc, then runs `func(arg)` from a `ThreadStartData`.
    extern "C" void* rpmallocThreadFunc(void* dataPtr);

    /// `pthread_key` destructor that halts rpmalloc for a thread; to be when the thread dies.
    extern "C" void rpmallocThreadHalt(void* keyValue);

    // Declared in "Globals.hh"
    void ARES_INIT_HOOK rpmallocInit()
    {
        if(!gRpmallocInited)
        {
            gRpmallocInited = true;

            // Set `rpmallocThreadHalt` to be run on death of each thread that
            // sets a value to `gThreadKey`
            pthread_key_create(&gThreadKey, rpmallocThreadHalt);

            bool ok = rpmalloc_initialize() == 0;
            if(!ok)
            {
                assert(false && "Failed to initialize rpmalloc");
                abort(); // Abort even in release: program can't allocate any memory!!
            }
        }
        rpmalloc_thread_initialize(); // Initialize main thread
    }

    // Declared in "Globals.hh"
    void ARES_HALT_HOOK rpmallocHalt()
    {
        // pthread_key_delete(gThreadKey); ?

        rpmalloc_thread_finalize(); // Finalize main thread
        if(gRpmallocInited)
        {
            gRpmallocInited = false;
            rpmalloc_finalize();
        }
    }


    void* rpmallocThreadFunc(void* dataPtr)
    {
        // Get copies of the passed func and arg, initialize rpalloc, then
        // deallocate the `ThreadStartData` with rpmalloc itself
        auto data = reinterpret_cast<ThreadStartData*>(dataPtr);
        auto threadFunc = data->func;
        auto threadArg = data->arg;
        rpmalloc_thread_initialize(); // (does nothing if already inited)
        rpfree(data);

        // Schedule `rpmallocThreadHalt` to run for this thread, too, by setting
        // this thread's key to a dummy value; the key is set to run
        // `rpmallocThreadHalt` as a destructor for the value, effectively running
        // it when the thread dies.
        pthread_setspecific(gThreadKey, (void*)42);

        // Run the actual thread function
        return threadFunc(threadArg);
    }

    void rpmallocThreadHalt(void* keyValue)
    {
        (void)keyValue;
        rpmalloc_thread_finalize(); // (does nothing if already finalized)
    }

}


// Now hook `rpmallocThreadInit()` to each thread created by `pthread_create`

using PFN_pthread_create = int(*)(pthread_t* thread, const pthread_attr_t* attr,
                                  void*(*func)(void*), void* arg);

#if defined(ARES_PLATFORM_IS_POSIX)
//  Non-Mac POSIX.
//  Implement `pthread_create`, ld will give it precedence over the system one.
//  In it, `dlsym()` the address of the real `pthread_create` and use it to launch
//  a rpmalloc-aware thread.
#   include <dlfcn.h>

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                              void*(*func)(void*), void* arg)
{
    rpmalloc_thread_initialize(); // Initalize the caller thread if not already done

    auto startData = (Ares::ThreadStartData*)rpmalloc(sizeof(Ares::ThreadStartData));
    startData->func = func;
    startData->arg = arg;

    auto sysPthreadCreate = (PFN_pthread_create)dlsym(RTLD_NEXT, "pthread_create");
    assert(sysPthreadCreate && "Failed to load pthread_create!");

    return sysPthreadCreate(thread, attr, Ares::rpmallocThreadFunc, startData);
    // `startData` will be deallocated by `rpmallocThreadFunc`
}

#elif defined(ARES_PLATFORM_IS_MAC)
//  Mac.
//  Implement a `ARES_pthread_create` and use dyld interposition to have it be
//  run instead of the system one

extern "C" int ARES_pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                                   void*(*func)(void*), void* arg)
{
    rpmalloc_thread_initialize(); // Initalize the caller thread if not already done

    auto startData = reinterpret_cast<ThreadStartData*>(rpmalloc(sizeof(ThreadStartData)));
    startData->func = func;
    startData->arg = arg;

    return pthread_create(thread, attr, rpmallocThreadFunc, startData);
    // `startData` will be deallocated by `rpmallocThreadFunc`
}


typedef extern "C" struct interpose_s
{
    void* new_func;
    void* orig_func;
} interpose_t;

#   define MAC_INTERPOSE(newf, oldf) __attribute__((used)) \
    static const interpose_t macinterpose##newf##oldf \
    __attribute__ ((section("__DATA, __interpose"))) = \
    { (void*)newf, (void*)oldf }

MAC_INTERPOSE(ARES_pthread_create, pthread_create);

#elif defined(ARES_PLATFORM_IS_WINDOWS)
// Windows + winpthreads and MinGW[-like] compiler.
//  Implement `pthread_create`, use it to shadow winpthreads' one.
//  In it, use the Win32 API to get the address of the real `pthread_create` in
//  winpthreads and launch a rpmalloc-aware thread.
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                              void*(*func)(void*), void* arg)
{
    rpmalloc_thread_initialize(); // Initalize the caller thread if not already done

    auto startData = (Ares::ThreadStartData*)rpmalloc(sizeof(Ares::ThreadStartData));
    startData->func = func;
    startData->arg = arg;

    HMODULE libwinpthread = GetModuleHandle("libwinpthread-1");
    assert(libwinpthread && "Failed to get a handle to libwinpthread-1.dll");

    auto sysPthreadCreate = (PFN_pthread_create)GetProcAddress(libwinpthread, "pthread_create");
    assert(sysPthreadCreate && "Failed to load pthread_create!");

    return sysPthreadCreate(thread, attr, Ares::rpmallocThreadFunc, startData);
    // `startData` will be deallocated by `rpmallocThreadFunc`
}

#else
#   error "Unsupported platform for pthread_create override!"
#endif
