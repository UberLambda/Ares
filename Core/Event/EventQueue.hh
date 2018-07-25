#pragma once

#include <assert.h>
#include <stddef.h>
#include <atomic>
#include <Core/Api.h>
#include <Core/Base/NumTypes.hh>

namespace Ares
{

/// The interface that all `EventQueue<T>`s implement.
class ARES_API EventQueueBase
{
public:
    virtual ~EventQueueBase() = default;

    /// Clears the queue, removing all events currently stored in it.
    virtual void clear() = 0;
};

/// A threadsafe, lockless queue of `T` events.
///
/// Iterators over `EventQueue<T>`s are safe to use even if elements are
/// added/removed from the queue while iterating. Worst case scenarios are that
/// either some newly-added elements don't get iterated over, or an iterator
/// returns a now-deleted element.
template <typename T>
class EventQueue : public EventQueueBase
{
    T* items_;
    size_t capacity_;
    std::atomic<size_t> used_;

public:
    friend class const_iterator;
    class const_iterator;

    /// Initializes a queue given its maximum event capacity.
    EventQueue(size_t capacity=1024)
        : capacity_(capacity), used_(0)
    {
        items_ = new T[capacity_];
    }

    ~EventQueue() override
    {
        delete[] items_; items_ = nullptr;
    }

    /// Returns the maximum capacity of the event queue.
    inline size_t capacity() const
    {
        return capacity_;
    }

    /// Returns the number of events currently stored in the event queue.
    /// Note that this value could differ if accessed from different threads at
    /// the same time!
    inline size_t used() const
    {
        return used_.load();
    }

    /// Pushes a new event to the queue.
    /// Threadsafe and lockless.
    /// **WARNING**: Asserts (or, in release, crashes the program) if the queue
    /// is full!
    void push(const T& event)
    {
        size_t slot = (used_ ++); // (atomic fetch/add)
        assert(slot < capacity_ && "Event queue full");
        items_[slot] = event;
    }

    /// Pushes `nEvents` new events to the queue from a forward iterator of `T`
    /// events.
    /// Threadsafe and lockless.
    /// **WARNING**: Asserts (or, in release, crashes the program) if the queue
    /// is full!
    template <typename TIterator>
    void push(TIterator events, size_t nEvents)
    {
        size_t firstSlot = std::atomic_fetch_add(&used_, nEvents);
        assert((firstSlot + nEvents) < capacity_ && "Event queue full");

        auto it = events;
        for(size_t i = firstSlot; i < firstSlot + nEvents; i ++)
        {
            items_[i] = *(it ++);
        }
    }

    /// Removes all items from the queue.
    /// Threadsafe and lockless.
    inline void clear() override
    {
        used_ = 0; // (atomic store)
    }


    /// A readonly iterator over a `EventQueue<T>`.
    class ARES_API const_iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using reference = const value_type&;
        using pointer = const value_type*;

    private:
        friend class EventQueue<T>;
        const EventQueue<T>* parent_;
        size_t index_;

        constexpr const_iterator(const EventQueue<T>* parent, size_t index)
            : parent_(parent), index_(index)
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
            index_ = toCopy.index_;

            return *this;
        }


        inline pointer operator->()
        {
            // NOTE: Referencing an item should never fail during a frame, since
            // the only way items can be removed is by `clear()`ing the whole
            // event list - an operation only done at the end of an update cycle
            // on the main thread, before a new frame starts!
            // The only case where it could actually fail is when trying to
            // dereference the one-past-the-end iterator returned by `EventQueue<T>::end()`!
            assert(index_ < parent_->used() && "Iterator index out of bounds");
            return &parent_->items_[index_];
        }

        inline reference operator*()
        {
            return *operator->();
        }


        inline bool operator==(const const_iterator& other) const
        {
            bool parentMatch = parent_ == other.parent_;

            // Special equality comparison: treat every out of bounds iterator as
            // equal. This way if something deletes items from the event queue
            // while it is being iterated the iteration will prematurely end even
            // if the `EventQueue<T>::end()` iterator that was required to iterate
            // the list in the first place differs.
            size_t parentMax = parent_->used_.load();
            bool indexMatch = index_ == other.index_ ||
                              (index_ >= parentMax && other.index_ >= parentMax);

            return parentMatch && indexMatch;
        }

        inline bool operator!=(const const_iterator& other) const
        {
            return !operator==(other);
        }


        const_iterator& operator++() // preincrement
        {
            index_ ++;
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
        return const_iterator(this, 0);
    }

    inline const_iterator cbegin() const
    {
        return begin();
    }

    inline const_iterator end() const
    {
        // Return the (for now) one-past-the-end iterator
        // Worst case scenario, if some events gets added to the parent queue
        // while it is being iterated, we will not iterate to some of the elements
        // in the queue.
        return const_iterator(this, used_.load());
    }

    inline const_iterator cend() const
    {
        return end();
    }
};

}
