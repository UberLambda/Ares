#pragma once

#include <utility>
#include "EntityId.hh"
#include "Scene.hh"

namespace Ares
{

/// A reference to an entity inside of a scene.
/// **WARNING**: Only valid up to the parent scene's destruction!
class EntityRef
{
    friend class Scene;

    Scene* scene_;
    EntityId id_;

    constexpr EntityRef(Scene* scene, EntityId id)
        : scene_(scene), id_(id)
    {
    }

public:
    EntityRef(const EntityRef& toCopy)
    {
        (void)operator=(toCopy);
    }

    EntityRef& operator=(const EntityRef& toCopy)
    {
        // Copy ref
        scene_ = toCopy.scene_;
        id_ = toCopy.id_;

        return *this;
    }


    EntityRef(EntityRef&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    EntityRef& operator=(EntityRef&& toMove)
    {
        // Copy ref
        scene_ = toMove.scene_;
        id_ = toMove.id_;

        // Invalidate the moved ref
        toMove.id_ = INVALID_ENTITY_ID;

        return *this;
    }


    ~EntityRef() = default;


    /// Returns a reference to the parent scene.
    inline const Scene& scene() const
    {
        return *scene_;
    }

    /// Returns the id of the underlying entity.
    inline EntityId id() const
    {
        return id_;
    }


    inline bool operator==(const EntityRef& other) const
    {
        return id_ == other.id_ && scene_ == other.scene_;
    }

    inline bool operator!=(const EntityRef& other) const
    {
        return !operator==(other);
    }


    /// Returns a pointer to the `T` component stored for this entity or null if
    /// there isn't one.
    /// **WARNING**: Not fully threadsafe! If the component is removed while the
    ///              `T*` is still in use, the pointer will now point to an unused
    ///              `T` **or may even point to a different entity's `T` component**!!
    template <typename T>
    inline T* comp()
    {
        auto store = scene_->storeFor<T>();
        if(store)
        {
            return store->get(id_);
        }
        else
        {
            return nullptr;
        }
    }

    /// Sets or replaces the `T` component stored for this entity and returns a
    /// pointer to the newly-stored component - or null if the component could
    /// not be set (entity id out of bounds?).
    /// **WARNING**: Not fully threadsafe! If the component is removed while the
    ///              `T*` is still in use, the pointer will now point to an
    ///              unused `T` **or may even point to a different entity's `T` component**!!
    template <typename T>
    inline T* setComp(T&& comp)
    {
        auto store = scene_->storeFor<T>();
        if(store)
        {
            return store->set(id_, std::move(comp));
        }
        else
        {
            return nullptr;
        }
    }

    /// Erases any `T` component stored for this entity.
    /// Does nothing if there isn't one (component not set or entity id out of bounds).
    /// **WARNING**: See `comp()`, `setComp()`'s warnings!
    template <typename T>
    inline void erase()
    {
        auto store = scene_->storeFor<T>();
        if(store)
        {
            store->erase(id_);
        }
    }

    /// Erases all components stored for this entity.
    /// Does nothing if the entity id is out of bounds.
    /// **WARNING**: See `comp()`, `setComp()`'s warnings!
    inline void eraseAll()
    {
        scene_->erase(id_);
    }
};

}
