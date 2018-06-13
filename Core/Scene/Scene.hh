#pragma once

#include <unordered_map>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include "Entity.hh"
#include "CompStore.hh"

namespace Ares
{

/// A collection of entities and the components associated to them.
class Scene
{
    size_t nEntities_;

    using CompStoreSlot = std::unique_ptr<CompStoreBase>;
    std::unordered_map<std::type_index, CompStoreSlot> compStores_;


    Scene(Scene&& toMove) = delete;
    Scene& operator=(Scene&& toMove) = delete;

    Scene(const Scene& toCopy) = delete;
    Scene& operator=(const Scene& toCopy) = delete;

public:
    /// Initializes an empty scene given the maximum number of entities that it
    /// could hold.
    Scene(size_t nEntities);
    ~Scene();

    /// Returns the maximum number of entities in the scene.
    inline size_t nEntities() const
    {
        return nEntities_;
    }


    /// Attempts to registers a scene component store for `T` components. Returns
    /// `false` if a component store for `T` was already registered.
    template <typename T>
    bool registerStoreFor()
    {
        auto slot = compStores_.find(typeid(T));
        if(slot != compStores_.end())
        {
            // CompStore<T> already registered
            return false;
        }

        compStores_[typeid(T)].reset(new CompStore<T>(nEntities_));
        return true;
    }

    /// Attempts to retrieve a pointer to the scene component store for `T`
    /// components; returns null if no such store has been registered in the scene.
    ///
    /// The pointer should remain valid throughout the lifetime of the scene, even
    /// if other stores are registered in the meantime.
    /// Do not call `delete`/`free()` on it by hand!
    template <typename T>
    CompStore<T>* storeFor()
    {
        auto slot = compStores_.find(typeid(T));
        if(slot == compStores_.end())
        {
            // CompStore<T> not registered
            return nullptr;
        }

        CompStoreBase* ptr = slot->second.get();
        return reinterpret_cast<CompStore<T>*>(ptr);
    }

    template <typename T>
    const CompStore<T>* storeFor() const
    {
        auto slot = compStores_.find(typeid(T));
        if(slot == compStores_.end())
        {
            // CompStore<T> not registered
            return nullptr;
        }

        const CompStoreBase* ptr = slot->second.get();
        return reinterpret_cast<const CompStore<T>*>(ptr);
    }


    /// Erases all components associated to `entity` across all scene component stores.
    void erase(Entity entity);

    /// Returns `true` if any scene component store has a component for `entity`.
    bool has(Entity entity) const;
};


}
