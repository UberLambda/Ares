#pragma once

#include <istream>
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
    ///
    /// Thread safe, but **not** lockless! This function is to be used sparingly
    /// at startup, not for being used in a critical path.
    template <typename T>
    ResourceRef<T> load(const Path& resPath)
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
                return ResourceRef<T>(this, {it->second});
            }
        }

        // Else we need to load it now; reserve an unique handle for the resource
        // and try getting a stream it from a stream file
        U32 resId = (nextResId_ ++); // (atomic)

        auto stream = fileStore_->getStream(resPath); // (assumed to be threadsafe)
        if(!stream)
        {
            // Fail: filestore could not find file
            return ResourceRef<T>{nullptr};
        }

        const char* fileExt = resPath.extension();

        // TODO Allocate resource in a contiguous memory region, not scattering
        //      them on the heap!
        auto resource = new TResHolder<T>();
        bool ok = ResourceParser<T>::parse(resource->t, *stream, fileExt);

        fileStore_->freeStream(stream); // (assumed to be threadsafe)

        if(ok)
        {
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

            return ResourceRef<T>{this, {resId}};
        }
        else
        {
            // Fail: resource stream could not be parsed to a resource
            delete resource;
            return ResourceRef<T>{nullptr};
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

template <typename T>
class Serializer; // (#include "Base/Serializer.hh"
                  //  `Serializer<ResourceRef<T>>` is implemented in "ResourceRefSerializer.hh")

template <typename T>
class ResourceRef
{
    friend class ResourceLoader;
    friend class Serializer<ResourceRef<T>>;

    ResourceLoader* parent_;
    ResourceHandle<T> handle_;

    ResourceRef(ResourceLoader* parent, ResourceHandle<T> handle={})
        : parent_(parent), handle_(handle)
    {
    }

public:
    ResourceRef(ResourceRef&& toMove)
    {
        (void)operator=(std::move(toMove));
    }
    ResourceRef& operator=(ResourceRef&& toMove)
    {
        // Move data over
        parent_ = toMove.parent_;
        handle_ = toMove.handle_;

        // Invalidate the moved instance
        toMove.parent_ = nullptr;

        return *this;
    }

    ResourceRef(const ResourceRef& toCopy)
    {
        (void)operator=(toCopy);
    }
    /// (Increments the reference count by 1; locks parent to do so)
    ResourceRef& operator=(const ResourceRef& toCopy)
    {
        // Copy data
        parent_ = toCopy.parent_;
        handle_ = toCopy.handle_;

        if(parent_)
        {
            parent_->incRefCount(handle_);
        }
    }

    /// (Decrements the reference count by 1; locks parent to do so)
    ~ResourceRef()
    {
        if(parent_)
        {
            parent_->decRefCount(handle_);
        }
    }


    /// Returns a pointer to the resource, or null if the resource is not in the
    /// parent anymore.
    ///
    /// Thread safe, but **not** lockless! This function is to be used sparingly,
    /// and not for being used in a critical path.
    inline const T* operator->() const
    {
        if(parent_ == nullptr || handle_ == -1)
        {
            return nullptr;
        }

        std::lock_guard<std::mutex> parentResMapScopedLock(parent_->resMapLock_);

        auto it = parent_->resMap_.find(handle_);
        if(it != parent_->resMap_.end())
        {
            // HACK The resource should always be a `T`, so we can upcast
            //      a `ResHolderBase` to a `TResHolder<T>` with a reinterpret cast
            //      here
            using TResHolder = ResourceLoader::TResHolder<T>;
            return &reinterpret_cast<TResHolder*>(it->second.resource)->t;
        }
        else
        {
            return nullptr;
        }
    }

    /// See `operator->()`.
    inline const T& operator*() const
    {
        return *operator->();
    }

    /// Returns a pointer to thre parent resource loader, or null if the reference
    /// is invalid.
    inline const ResourceLoader* parent() const
    {
        return parent_;
    }

    /// Returns the handle associated to the reference, which will be invalid if
    /// the reference is invalid.
    inline ResourceHandle<T> handle() const
    {
        return handle_;
    }

    /// Returns the current reference count of the pointed to resource in the parent
    ///
    /// Thread safe, but **not** lockless! This function is to be used sparingly,
    /// and not for being used in a critical path.
    inline unsigned int refCount()  const
    {
        return parent_ ? parent_->getRefCount(handle_) : 0;
    }
};

}
