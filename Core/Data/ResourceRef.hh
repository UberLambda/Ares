#pragma once

#include "ResourceHandle.hh"

// #include "ResourceLoader.hh" before using this file!

namespace Ares
{

template <typename T>
class Serializer; // (#include "Base/Serializer.hh"
                  //  `Serializer<ResourceRef<T>>` is implemented in "ResourceRefSerializer.hh")


/// A smart pointer to a `T` resource associated to a `ResourceLoader`, fully
/// reference-counted.
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
