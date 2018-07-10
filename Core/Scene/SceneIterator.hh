#pragma once

#include "Scene.hh"
#include "EntityRef.hh"

namespace Ares
{

class Scene::iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = EntityRef;
    using reference = const value_type&;
    using pointer = const value_type*;

private:
    friend class Scene;
    Scene* parent_;
    value_type ref_;


    constexpr iterator(Scene* parent, EntityId entity)
        : parent_(parent),
          ref_{parent_, entity}
    {
    }

public:
    iterator(const iterator& toCopy)
        : parent_(toCopy.parent_), ref_(toCopy.ref_)
    {
    }

    inline iterator& operator=(const iterator& toCopy)
    {
        parent_ = toCopy.parent_;
        ref_ = toCopy.ref_;

        return *this;
    }


    inline reference operator*()
    {
        return ref_;
    }


    inline bool operator==(const iterator& other) const
    {
        return ref_ == other.ref_ && parent_ == other.parent_;
        // (Note: if `pair_.entity` matches, then `pair_.component` should also)
    }

    inline bool operator!=(const iterator& other) const
    {
        return !operator==(other);
    }


    iterator& operator++() // preincrement
    {
        // Go to the next entity
        ref_.id_ ++;

        return *this;
    }

    inline iterator operator++(int) // postincrement
    {
        iterator old = *this;
        (void)operator++();
        return old;
    }
};

}
