#pragma once

#include <stddef.h>
#include <utility>
#include <iterator>
#include "../Base/Utils.hh"
#include "Entity.hh"

namespace Ares
{

/// The interface containing common functionality of all `CompStore<T>`s.
class CompStoreBase
{
public:
    virtual ~CompStoreBase() = default;

    /// Attempts to erase the component associated to `entity`. Returns `false`
    /// on error (`entity` is out of bounds or no component to delete for `entity`).
    virtual bool erase(Entity entity) = 0;

    /// Returns `true` if no component for any entity is currently stored.
    virtual bool empty() const = 0;
};

/// A sparse collection of `T` components indexed by `Entity`.
/// Supports iteration.
template <typename T>
class CompStore : public CompStoreBase
{
    size_t nEntities_;
    bool* compMap_;
    T* compData_;
    Entity firstSetEntity_, lastSetEntity_;

    CompStore(const CompStore& toCopy) = delete;
    CompStore& operator=(const CompStore& toCopy) = delete;

public:
    friend class const_iterator;
    class const_iterator;

    /// Initializes a component store with capacity for a certain number of
    /// entities. The maximum `Entity` index will hence be `nEntities - 1`.
    CompStore(size_t nEntities)
        : nEntities_(nEntities),
          firstSetEntity_(-1), // Set this initially to -1; see `set()`
          lastSetEntity_(0) // Set this initially to 0; see `set()`
    {
        compMap_ = new bool[nEntities_];
        compData_ = new T[nEntities_];
    }

    CompStore(CompStore&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    CompStore& operator=(CompStore&& toMove)
    {
        // Move data over
        nEntities_ = toMove.nEntities_;
        compMap_ = std::move(toMove.compMap_);
        compData_ = std::move(toMove.compData_);
        firstSetEntity_ = toMove.firstSetEntity_;
        lastSetEntity_ = toMove.lastSetEntity_;

        // Invalidate moved instance
        toMove.compMap_ = nullptr;
        toMove.compData_ = nullptr;

        return *this;
    }

    ~CompStore() override
    {
        delete[] compMap_; compMap_ = nullptr;
        delete[] compData_; compData_ = nullptr;
    }

    /// Returns `true` if the component store is valid (not moved, not destroyed)
    /// or `false` otherwise.
    inline operator bool() const
    {
        return compMap_ && compData_;
    }


    /// Attempts to get the component associated to `entity`. Returns `false`
    /// on error (`entity` is out of bounds or no component to get for `entity`).
    inline bool get(T& outComp, Entity entity) const
    {
        if(entity < nEntities_ && compMap_[entity])
        {
            outComp = compData_[entity];
            return true;
        }
        else
        {
            // Entity out of bounds or component not present
            return false;
        }
    }

    /// Attempts to set the component associated to `entity` to a copy of `comp`.
    /// Returns `false` on error (`entity` is out of bounds).
    inline bool set(T comp, Entity entity)
    {
        if(entity >= nEntities_)
        {
            // Entity out of bounds
            return false;
        }

        compMap_[entity] = true;
        compData_[entity] = comp;

        // Maybe this changed the first or last set entity
        // NOTE: `lastSetEntity_` is initially set to 0 so that `max` will work
        //       properly the first time (0 < any other entity), while
        firstSetEntity_ = min(firstSetEntity_, entity);
        lastSetEntity_ = max(lastSetEntity_, entity);

        return true;
    }

    /// Attempts to erase the component associated to `entity`. Returns `true`
    /// on success or `false` otherwise (`entity` is out of bounds or no component
    /// to delete for `entity`).
    bool erase(Entity entity) override
    {
        if(entity < nEntities_ && compMap_[entity])
        {
            compMap_[entity] = false;

            if(entity == firstSetEntity_)
            {
                // Deleted the first set entity; check the first one to be found.
                // If none is left, reset `firstSetEntity_` to `Entity(-1)` (see `set()`)
                firstSetEntity_ = Entity(-1);
                for(Entity i = entity + 1; i < lastSetEntity_; i ++)
                {
                    if(compMap_[i])
                    {
                        firstSetEntity_ = i;
                        break;
                    }
                }
            }
            if(entity == lastSetEntity_)
            {
                // Deleted the last set entity; check the last one to be found.
                // If none is left, reset `lastSetEntity_` to `0` (see `set()`)
                lastSetEntity_ = 0;

                // Note: iterating from `entity - 1` would cause an underflow
                //       for `entity == 0`, so we need to start from `entity`.
                //       `compMap_[entity]` will always be `false` since it has
                //        just been deleted, so it's just an extra iteration.
                for(Entity i = entity; i >= firstSetEntity_; i ++)
                {
                    if(compMap_[i])
                    {
                        lastSetEntity_ = i;
                        break;
                    }
                }
            }

            return true;
        }
        else
        {
            // Entity out of bounds or component not present
            return false;
        }
    }

    /// Returns `true` if no component for any entity is currently stored.
    inline bool empty() const override
    {
        // Note: `Entity(-1) > Entity(0)` (`Entity` is an unsigned integer)
        return firstSetEntity_ > lastSetEntity_;
    }


    class const_iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<Entity, const T*>;
        using reference = const value_type&;
        using pointer = const value_type*;

    private:
        friend class CompStore;
        const CompStore* parent_;
        value_type pair_; // Entity => const T*


        constexpr const_iterator(const CompStore* parent, Entity entity)
            : parent_(parent),
              pair_{entity, nullptr}
        {
        }

    public:
        constexpr const_iterator(const const_iterator& toCopy)
        {
            (void)operator=(toCopy);
        }

        inline const_iterator& operator=(const const_iterator& toCopy)
        {
            parent_ = toCopy.parent_;
            pair_.first = toCopy.pair_.first;
            pair_.second = toCopy.pair_.second;

            return *this;
        }


        inline reference operator*()
        {
            // Update the `T*` pointer before returning the pair
            if(pair_.first != INVALID_ENTITY && parent_->compMap_[pair_.first])
            {
                pair_.second = &parent_->compData_[pair_.first];
            }
            else
            {
                // `INVALID_ENTITY` iterator or component has been erased
                pair_.second = nullptr;
            }

            return pair_;
        }


        inline bool operator==(const const_iterator& other) const
        {
            return pair_.first == other.pair_.first && parent_ == other.parent_;
            // (Note: if `pair_.first` matches, then `pair_.second` should also)
        }

        inline bool operator!=(const const_iterator& other) const
        {
            return pair_.first != other.pair_.first || parent_ != other.parent_;
            // (Note: if `pair_.first` does not match, then `pair_.second` also should not)
        }


        const_iterator& operator++() // preincrement
        {
            // Search for the next entity, if any
            Entity nextEntity = parent_->lastSetEntity_ + 1;
            for(Entity i = pair_.first + 1; i <= parent_->lastSetEntity_; i ++)
            {
                if(parent_->compMap_[i])
                {
                    // Next entity found
                    nextEntity = i;
                    break;
                }
            }

            // (`second` will be updated each time `operator*()` is called)
            pair_.first = nextEntity;
            pair_.second = nullptr;

            return *this;
        }

        inline const_iterator operator++(int) // postincrement
        {
            const_iterator old = *this;
            (void)operator++();
            return old;
        }
    };

    inline const_iterator begin() const
    {
        return const_iterator(this, empty() ? INVALID_ENTITY : firstSetEntity_);
    }

    inline const_iterator cbegin() const
    {
        return begin();
    }

    inline const_iterator end() const
    {
        return const_iterator(this, empty() ? INVALID_ENTITY : lastSetEntity_ + 1);
    }

    inline const_iterator cend() const
    {
        return end();
    }
};

}
