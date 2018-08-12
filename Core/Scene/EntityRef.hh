#pragma once

#include <utility>
#include <Core/Api.h>
#include <Core/Scene/EntityId.hh>
#include <Core/Scene/Scene.hh>

namespace Ares
{

/// A nullable reference to an entity inside of a scene.
/// **WARNING**: Only valid up to the parent scene's destruction!
class ARES_API EntityRef
{
    friend class Scene;

    Scene* scene_;
    EntityId id_;

    constexpr EntityRef(Scene* scene, EntityId id)
        : scene_(scene), id_(id)
    {
    }

public:
    /// Initializes a new null reference.
    EntityRef(std::nullptr_t null=nullptr)
        : scene_(nullptr), id_(INVALID_ENTITY_ID)
    {
    }

    /// Returns `false` if the `EntityRef` is null.
    inline operator bool() const
    {
        return scene_ && id_ != INVALID_ENTITY_ID;
    }



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


    /// Returns a pointer to the parent scene.
    /// Returns null for a null `EntityRef`.
    inline const Scene* scene() const
    {
        return scene_;
    }

    /// Returns the id of the underlying entity.
    /// Returns `INVALID_ENTITY_ID` for a null `EntityRef`.
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
    /// **WARNING**: The `EntityRef` should not be null!
    /// **WARNING**: Not fully threadsafe! If the component is removed while the
    ///              `T*` is still in use, the pointer will now point to an unused
    ///              `T` **or may even point to a different entity's `T` component**!!
    template <typename T>
    inline T* comp()
    {
        auto store = scene_->storeFor<T>(); // (gets added if it does not already exist)
        return store->get(id_);
    }

    /// Sets or replaces the `T` component stored for this entity and returns a
    /// pointer to the newly-stored component - or null if the component could
    /// not be set (entity id out of bounds?).
    /// **WARNING**: The `EntityRef` should not be null!
    /// **WARNING**: Not fully threadsafe! If the component is removed while the
    ///              `T*` is still in use, the pointer will now point to an
    ///              unused `T` **or may even point to a different entity's `T` component**!!
    template <typename T>
    inline T* setComp(T&& comp)
    {
        auto store = scene_->storeFor<T>(); // (gets added if it does not already exist)
        return store->set(id_, std::move(comp));
    }

    /// Erases any `T` component stored for this entity.
    /// Does nothing if there isn't one (component not set or entity id out of bounds).
    /// **WARNING**: The `EntityRef` should not be null!
    /// **WARNING**: See `comp()`, `setComp()`'s warnings!
    template <typename T>
    inline void erase()
    {
        auto store = scene_->storeFor<T>(); // (gets added if it does not already exist)
        store->erase(id_);
    }

    /// Erases all components stored for this entity.
    /// Does nothing if the entity id is out of bounds.
    /// **WARNING**: The `EntityRef` should not be null!
    /// **WARNING**: See `comp()`, `setComp()`'s warnings!
    inline void eraseAll()
    {
        scene_->erase(id_);
    }
};

}
