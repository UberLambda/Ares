#pragma once

#include <Core/Data/Path.hh>

#include <Core/Base/Platform.h>
#ifdef ARES_PLATFORM_IS_WINDOWS
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <dlfcn.h>
#endif

namespace Ares
{

/// A wrapper over a native shared library.
class Dll
{
    Path path_;
    bool doNotFree_;
    void* handle_;

    Dll(const Dll& toCopy) = delete;
    Dll& operator=(const Dll& toCopy) = delete;

public:
    /// Creates a new `Dll` without actually loading any library.
    Dll()
        : handle_(nullptr), doNotFree_(false)
    {
    }

    /// Initializes a `Dll` by attempting to load the library at `path`.
    /// Check `operator bool()` afterwards to see if the operation succeeded.
    ///
    /// If `doNotFree` is `true`, the library will not be freed/closed when
    /// `~Dll()` is run! This is useful to keep a library open for the whole lifetime
    /// of a program; the OS will unload them afterwards anyways
    Dll(const Path& path, bool doNotFree=false)
        : handle_(nullptr), path_(path), doNotFree_(doNotFree)
    {
        reload();
    }

    /// Unloads any currently-loaded library, then attempts to reload the library
    /// at `path()`. Returns `false` on failure (sets `operator bool()` to `false` as well).
    bool reload()
    {
        (void)this->~Dll();

#ifdef ARES_PLATFORM_IS_WINDOWS
        handle_ = (void*)LoadLibrary(path_.str().c_str());
#else
        handle_ = dlopen(path_, RTLD_LAZY);
#endif
        return operator bool();
    }

    Dll(Dll&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    Dll& operator=(Dll&& toMove)
    {
        // Move data over
        path_ = std::move(toMove.path_);
        handle_ = std::move(toMove.handle_);

        // Invalidate the moved instance
        toMove.handle_ = nullptr;

        return *this;
    }

    /// Unloads any loaded library if `doNotFree` was not set.
    ~Dll()
    {
        if(handle_ && !doNotFree_)
        {
#ifdef ARES_PLATFORM_IS_WINDOWS
            FreeLibrary((HMODULE)handle_);
#else
            dlclose(handle_);
#endif
            handle_ = nullptr;
        }
    }


    /// Returns `true` if a library is currently successfully loaded (and has
    /// not been unloaded by `~Dll()`).
    inline operator bool() const
    {
        return handle_ != nullptr;
    }

    /// Returns the path to the currently-loaded (or attempted to load) library.
    /// Returns an empty path if no library was ever attempted to be loaded.
    inline const Path& path() const
    {
        return path_;
    }

    /// Returns the handle to the currently-loaded native shared library.
    inline const void* handle() const
    {
        return handle_;
    }


    /// Returns a pointer to the symbol with name `name` in the library, or null
    /// on error (symbol not found or library not loaded).
    void* symbol(const char* name) const
    {
        if(!handle_)
        {
            return nullptr;
        }

#ifdef ARES_PLATFORM_IS_WINDOWS
        return (void*)GetProcAddress((HMODULE)handle_, name);
#else
        return dlsym(handle_, name);
#endif
    }
};

}
