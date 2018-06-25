#pragma once

#include <istream>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <type_traits>
#include <atomic>
#include <mutex>
#include "../Base/NumTypes.hh"
#include "FileStore.hh"
#include "ResourceHandle.hh"
#include "ResourceParser.hh"

namespace Ares
{

template <typename T>
class ResourceRef;

/// A loader of resources.
class ResourceLoader
{
    template <typename T>
    friend class ResourceRef;

    FileStore* fileStore_;


    /// Used for type erasure on `TResHolder<T>`s.
    struct ResHolderBase
    {
        virtual ~ResHolderBase() = default;
    };

    /// A struct that only contains a single `t` resource; see `cleanup()` for
    /// why this is needed.
    template <typename T>
    struct TResHolder : public ResHolderBase
    {
        T t;

        ~TResHolder() override = default; // (`resource.~T()` will be run after this)
    };

    /// A currently-loaded resource.
    struct LoadedResource
    {
        unsigned int refCount; ///< The resource's reference count, 0 = resource can be freed.
        ResHolderBase* resource; ///< A resource holder allocated on the heap for the `T` resource.
    };

    std::unordered_map<Path, U32> pathMap_; ///< A map of `resource path -> resource handle`
    std::unordered_map<U32, LoadedResource> resMap_; ///< A map of `resource handle -> loaded resource`
    std::mutex pathMapLock_, resMapLock_; ///< Locks around `pathMap_` and `resMap_` for thread safety.
    std::atomic<U32> nextResId_; ///< The id that will be assigned to the next loaded resource.


    /// Increases the reference count for the resource with the given handle.
    void incRefCount(U32 handle)
    {
        std::lock_guard<std::mutex> resMapScopedLock(resMapLock_);
        resMap_[handle].refCount ++;
    }

    /// Decreases the reference count for the resource with the given handle.
    void decRefCount(U32 handle)
    {
        std::lock_guard<std::mutex> resMapScopedLock(resMapLock_);
        resMap_[handle].refCount --;
    }

    /// Gets the reference count of the resource with the given handle. Returns `0`
    /// if the resource could not be found.
    unsigned int getRefCount(U32 handle)
    {
        std::lock_guard<std::mutex> resMapScopedLock(resMapLock_);
        auto it = resMap_.find(handle);
        if(it != resMap_.end())
        {
            return it->second.refCount;
        }
        else
        {
            return 0;
        }
    }

public:
    ResourceLoader(FileStore* fileStore);
    ~ResourceLoader();

    /// Attempts to parse and load a resource of type `T` in memory.
    /// Returns a non-empty `ErrString` on error, leaving `outRef` unchanged.
    ///
    /// Thread safe, but **not** lockless! This function is to be used sparingly
    /// at startup, not for being used in a critical path.
    template <typename T>
    ErrString load(ResourceRef<T>& outRef, const Path& resPath)
    {
        // Find wether the resource is already loaded or not; if it is, increment
        // its reference count and return a new reference to it
        {
            std::lock_guard<std::mutex> scopedPathMapLock(pathMapLock_);

            auto it = pathMap_.find(resPath);
            if(it != pathMap_.end())
            {
                LoadedResource& res = resMap_[it->second];
                res.refCount ++;
                outRef = ResourceRef<T>(this, {it->second});
                return ErrString(); // (no error)
            }
        }

        // Else we need to load it now; reserve an unique handle for the resource
        // and try getting a stream it from a stream file
        U32 resId = (nextResId_ ++); // (atomic)

        auto stream = fileStore_->getStream(resPath); // (assumed to be threadsafe)
        if(!stream)
        {
            std::ostringstream errStr;
            errStr << "Filestore could not find resource file: " << resPath;
            return errStr.str();
        }

        const char* fileExt = resPath.extension();

        // TODO Allocate resource in a contiguous memory region, not scattering
        //      them on the heap!
        auto resource = new TResHolder<T>();
        ErrString parsingErr = ResourceParser<T>::parse(resource->t, *stream, fileExt);

        fileStore_->freeStream(stream); // (assumed to be threadsafe)

        if(!parsingErr)
        {
            // FIXME May potentially deadlock if `pathMapLock_` and/or `resMapLock_`
            //       is locked somewhere else! Maybe merge the two locks into a single one?

            // Resource loaded; mark its `path -> handle` relationship on `pathMap_`,
            // then put the resource on `resMap_`; return a handle with refcount 1
            pathMapLock_.lock();
            pathMap_[resPath] = resId;
            pathMapLock_.unlock();

            resMapLock_.lock();
            LoadedResource& res = resMap_[resId];
            res.refCount = 1; // (atomic)
            res.resource = resource;
            resMapLock_.unlock();

            outRef = ResourceRef<T>{this, {resId}};
            return ErrString(); // (no error)
        }
        else
        {
            delete resource;

            std::ostringstream errStr;
            errStr << "Resource stream could not be parsed to a resource: " << parsingErr;
            return errStr.str();
        }
    }

    /// Returns `true` if a resource at the given path is currently loaded.
    ///
    /// Thread safe, but **not** lockless! This function is to be used sparingly
    /// at startup, not for being used in a critical path.
    bool isLoaded(const Path& resPath)
    {
        std::lock_guard<std::mutex> pathMapScopedLock(pathMapLock_);
        return pathMap_.find(resPath) != pathMap_.end();
    }

    /// Free all resources whose `refCount` is 0. Resturns the number of resources
    /// freed.
    ///
    /// Thread safe, but **definitely not** lockless (will lock the entire `ResourceLoader`)!
    /// This function is to be used sparingly, not for being used in a critical path.
    size_t cleanup();
};

}

// (#include "ResourceRef.hh" to use `ResourceLoader::load<T>()`)
