#pragma once

#include <unordered_map>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <mutex>
#include "EntityId.hh"
#include "CompStore.hh"

namespace Ares
{

class EntityRef; // #include "EntityRef.hh"

/// A collection of entities and the components associated to them.
class Scene
{
    friend class EntityRef;

    size_t maxEntities_;

    using CompStoreSlot = std::unique_ptr<CompStoreBase>;
    std::unordered_map<std::type_index, CompStoreSlot> compStores_;
    std::mutex compStoresLock_;

    Scene(Scene&& toMove) = delete;
    Scene& operator=(Scene&& toMove) = delete;

    Scene(const Scene& toCopy) = delete;
    Scene& operator=(const Scene& toCopy) = delete;

public:
    friend class iterator;
    class iterator;  // #include "SceneIterator.hh"

    /// Initializes an empty scene given the maximum number of entities that it
    /// could hold.
    Scene(size_t maxEntities);
    ~Scene();

    /// Returns the maximum number of entities in the scene.
    inline size_t maxEntities() const
    {
        return maxEntities_;
    }


    /// Gets a pointer to the scene's component store for `T`s.
    /// If no store for `T`s was already allocated, creates a new one before
    /// returning a pointer to it.
    ///
    /// The pointer should remain valid throughout the lifetime of the scene, even
    /// if other stores are registered in the meantime.
    /// Do not call `delete`/`free()` on it by hand!
    template <typename T>
    CompStore<T>* storeFor()
    {
        std::lock_guard<std::mutex> compStoresScopedLock(compStoresLock_);

        auto slot = compStores_.find(typeid(T));
        if(slot == compStores_.end())
        {
            // CompStore<T> not present, create one now
            auto storeBasePtr = std::unique_ptr<CompStoreBase>(new CompStore<T>(maxEntities_));
            slot = compStores_.insert(std::make_pair(std::type_index(typeid(T)),
                                                     std::move(storeBasePtr))).first;
        }

        CompStoreBase* ptr = slot->second.get();
        return reinterpret_cast<CompStore<T>*>(ptr);
    }


    /// Returns a reference to the entity with the given id in this scene.
    /// **WARNING**: Referencing an out-of-bounds entity has undefined consequences!
    EntityRef ref(EntityId entity);

    /// Returns `true` if any component is associated to `entity` in any store.
    bool has(EntityId entity);

    /// Erases all components associated to `entity` across all scene component stores.
    void erase(EntityId entity);


    /// Returns the begin iterator over all entities in the scene.
    iterator begin();

    /// Returns the end iterator over all entities in the scene.
    iterator end();
};

}
