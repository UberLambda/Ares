#pragma once

#include <stddef.h>
#include <vector>
#include <utility>
#include <iterator>
#include <Core/Api.h>
#include <Core/Base/Utils.hh>
#include <Core/Scene/EntityId.hh>

namespace Ares
{

/// The interface containing common functionality of all `CompStore<T>`s.
class CompStoreBase
{
public:
    virtual ~CompStoreBase() = default;

    /// Attempts to erase the component associated to `entity`.
    /// Does nothing if it is not present.
    virtual void erase(EntityId entity) = 0;

    /// Returns `true` if a component is currently associated to `entity` in the
    /// store.
    virtual bool has(EntityId entity) = 0;
};

/// A sparse collection of `T` components indexed by `Entity`.
/// Supports iteration.
template <typename T>
class ARES_API CompStore : public CompStoreBase
{
    size_t maxEntities_;
    std::vector<bool> compMap_; // (note: specialized in the STL)
                                // TODO Make `compMap_` use atomic bools?
    std::vector<T> compData_;

    CompStore(const CompStore& toCopy) = delete;
    CompStore& operator=(const CompStore& toCopy) = delete;

public:
    friend class iterator;
    class iterator;

    /// Initializes a component store with capacity for a certain number of
    /// entities. The maximum `Entity` index will hence be `maxEntities - 1`.
    CompStore(size_t maxEntities)
        : maxEntities_(maxEntities),
          compMap_(maxEntities), compData_(maxEntities)
    {
    }

    CompStore(CompStore&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    CompStore& operator=(CompStore&& toMove)
    {
        // Move data over and invalidate moved instance
        maxEntities_ = toMove.maxEntities_;
        compMap_ = std::move(toMove.compMap_);
        compData_ = std::move(toMove.compData_);

        return *this;
    }

    ~CompStore() override = default;


    /// Returns a pointer to the component stored for `entity` or null if
    /// there isn't one.
    /// **WARNING**: Not fully threadsafe! If the component is removed while the
    ///              `T*` is still in use, the pointer will now point to an unused
    ///              `T` **or may even point to a different entity's `T` component**!!
    inline T* get(EntityId entity)
    {
        if(entity < maxEntities_ && compMap_[entity])
        // TODO Make `compMap_[entity]` be an atomic load?
        {
            return &compData_[entity];
        }
        else
        {
            // Entity out of bounds or component not set
            return nullptr;
        }
    }

    /// Sets or replaces the `T` component stored for this entity and returns a
    /// pointer to the newly-stored component - or null if the component could not be
    /// set (possibly because `entity` is out of bounds).
    /// **WARNING**: Not fully threadsafe! If the component is removed while the
    ///              `T*` is still in use, the pointer will now point to an unused
    ///              `T` **or may even point to a different entity's `T` component**!!
    inline T* set(EntityId entity, T&& comp)
    {
        if(entity >= maxEntities_)
        {
            // Entity out of bounds
            return nullptr;
        }

        compMap_[entity] = true; // TODO Make this be an atomic store?
        T* compPtr = &compData_[entity];
        *compPtr = std::move(comp);
        return compPtr;
    }

    /// Returns `true` if a component is currently associated to `entity` in the
    /// store.
    /// **WARNING**: Not reliable in multithreaded environments; see `comp()`,
    ///              `setComp()`'s warnings!
    inline bool has(EntityId entity) override
    {
        return entity < maxEntities_ && compMap_[entity];
    }

    /// Attempts to erase the component associated to `entity`.
    /// Does nothing if there isn't one (component not set or entity id out of bounds).
    /// **WARNING**: See `comp()`, `setComp()`'s warnings!
    inline void erase(EntityId entity) override
    {
        if(entity < maxEntities_)
        {
            // Erase component now or keep its "erased" value `false`
            compMap_[entity] = false;
        }
    }


    class ARES_API iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        struct value_type
        {
            EntityId entity;
            T* component;
        };
        using reference = const value_type&;
        using pointer = const value_type*;

    private:
        friend class CompStore;
        CompStore* parent_;
        value_type pair_;


        constexpr iterator(CompStore* parent, EntityId entity)
            : parent_(parent),
              pair_{entity, nullptr}
        {
        }

    public:
        constexpr iterator(const iterator& toCopy)
        {
            (void)operator=(toCopy);
        }

        inline iterator& operator=(const iterator& toCopy)
        {
            parent_ = toCopy.parent_;
            pair_ = {toCopy.pair_.entity, toCopy.pair_.value};

            return *this;
        }


        inline reference operator*()
        {
            // Update the `T*` pointer before returning the pair
            if(pair_.entity != INVALID_ENTITY_ID && parent_->compMap_[pair_.entity])
            {
                pair_.component = &parent_->compData_[pair_.first];
            }
            else
            {
                // `INVALID_ENTITY` iterator or component has been erased
                pair_.component = nullptr;
            }

            return pair_;
        }


        inline bool operator==(const iterator& other) const
        {
            return pair_.entity == other.pair_.entity && parent_ == other.parent_;
            // (Note: if `pair_.entity` matches, then `pair_.component` should also)
        }

        inline bool operator!=(const iterator& other) const
        {
            return !operator==(other);
        }


        iterator& operator++() // preincrement
        {
            // Search for the next entity, if any
            EntityId nextEntity = INVALID_ENTITY_ID;
            for(EntityId i = pair_.entity; i < parent_->maxEntities_; i ++)
            {
                if(parent_->compMap_[i])
                {
                    // Next entity found
                    nextEntity = i;
                    break;
                }
            }

            // (`pair_.component` will be updated each time `operator*()` is called)
            pair_.entity = nextEntity;

            return *this;
        }

        inline iterator operator++(int) // postincrement
        {
            iterator old = *this;
            (void)operator++();
            return old;
        }
    };

    inline iterator begin()
    {
        return iterator(this, 0);
    }

    inline iterator end()
    {
        return iterator(this, maxEntities_);
    }
};

}
