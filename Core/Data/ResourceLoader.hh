#pragma once

#include <utility>
#include <mutex>
#include <vector>
#include <sstream>
#include <unordered_map>
#include "../Base/Ref.hh"
#include "../Base/TypeMap.hh"
#include "FileStore.hh"
#include "ResourceParser.hh"

namespace Ares
{

/// A loader of resources.
class ResourceLoader
{
    struct ResourceStoreBase
    {
        virtual ~ResourceStoreBase() = default;

        /// Returns `true` if a resource at `path` is currently loaded in the store.
        virtual bool isLoaded(const Path& path) = 0;

        /// Cleans up all resources whose refcount is 1 from the store, i.e.
        /// resources loaded in the store but never referenced outside of it.
        virtual size_t cleanup() = 0;
    };

    template <typename T>
    struct ResourceStore : public ResourceStoreBase
    {
        ~ResourceStore() override = default;

        std::unordered_map<Path, Ref<T>> resourceMap;
        std::mutex resourceMapLock;

        bool isLoaded(const Path& path) override
        {
            std::lock_guard<std::mutex> lock(resourceMapLock); // Lock `resourceMap`
            return resourceMap.find(path) != resourceMap.end();
        }

        size_t cleanup() override
        {
            std::lock_guard<std::mutex> lock(resourceMapLock); // Lock `resourceMap`

            size_t nCleaned = 0;
            for(auto it = resourceMap.begin(); it != resourceMap.end();)
            {
                if(it->second.refCount() == 1)
                {
                    it = resourceMap.erase(it);
                    nCleaned ++;
                }
                else
                {
                    it ++;
                }
            }

            return nCleaned;
        }
    };

    Ref<FileStore> fileStore_;
    TypeMap resourceStores_;
    std::mutex resourceStoresLock_;

public:
    ResourceLoader(Ref<FileStore> fileStore);
    ~ResourceLoader();


    /// Returns a reference to the resource loaders' underlying file store.
    inline Ref<FileStore> fileStore()
    {
        return fileStore_;
    }


    /// Attempts to parse and load a resource of type `T` in memory.
    /// Returns a non-empty `ErrString` on error, leaving `outRef` unchanged.
    ///
    /// Thread safe, but **not** lockless! This function is to be used sparingly
    /// at startup, not for being used in a critical path.
    template <typename T>
    ErrString load(Ref<T>& outRef, const Path& resPath)
    {
        ResourceStore<T>* store;

        {
            // `resourceStores` lock scope
            std::lock_guard<std::mutex> storesLock(resourceStoresLock_);

            store = resourceStores_.get<ResourceStore<T>>();
            if(!store)
            {
                // `ResourceStore` for Ts missing, create one
                (void)resourceStores_.add<ResourceStore<T>>();
                store = resourceStores_.get<ResourceStore<T>>();
            }
        }

        {
            // `store` lock scope
            std::lock_guard<std::mutex> storeLock(store->resourceMapLock);

            auto it = store->resourceMap.find(resPath);
            if(it != store->resourceMap.end())
            {
                // Resource already loaded, return a new (refcounted) reference to it
                outRef = it->second; // (ref copy: increases reference count by one)
                return {};
            }
        }

        // Else we need to load the resource now
        auto stream = fileStore_->getStream(resPath); // (assumed to be threadsafe)
        if(!stream)
        {
            std::ostringstream errStr;
            errStr << "Filestore could not find resource file: " << resPath;
            return errStr.str();
        }

        // Allocate a refcounted `T` and try to parse it from file
        // TODO Allocate resource in a contiguous memory region, not scattering
        //      them on the heap!
        auto resource = makeRef<T>();
        ErrString parsingErr = ResourceParser<T>::parse(*resource, *stream, resPath,
                                                        *this);

        fileStore_->freeStream(stream); // (assumed to be threadsafe)

        if(!parsingErr)
        {
            // Resource loaded, put it into the store return a reference to it
            // `store` lock scope
            std::lock_guard<std::mutex> storeLock(store->resourceMapLock);

            store->resourceMap[resPath] = resource; // (ref copy: increases reference count to 2)

            outRef = std::move(resource); // (leaves reference count unchanged)
            return {};
        }
        else
        {
            return parsingErr;
            // (`~Ref()` will automatically deallocate the resource)
        }
    }

    /// Returns `true` if a resource of any type at the given path is loaded.
    ///
    /// Thread safe, but **not** lockless! This function is to be used sparingly
    /// at startup, not for being used in a critical path.
    bool isLoaded(const Path& resPath);

    /// Free all resources whose reference count is 1, i.e. that are stored in
    /// the loader but never referenced outside of it. Returns the number of
    /// resources freed.
    ///
    /// Thread safe, but **definitely not** lockless (will lock the entire `ResourceLoader`)!
    /// This function is to be used sparingly, not for being used in a critical path.
    size_t cleanup();
};

}

// (#include "ResourceRef.hh" to use `ResourceLoader::load<T>()`)
