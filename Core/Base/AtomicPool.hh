#pragma once

#include <stddef.h>
#include <assert.h>
#include "AtomicArray.hh"
#include "NumTypes.hh"

namespace Ares
{

/// An pool of `T`s where grabbing/freeing a `T` from the pool is a thread-safe
/// atomic operation.
template <typename T>
class AtomicPool
{
    size_t INVALID_INDEX = -1;

    size_t n_;
    T* items_;
    AtomicArray<bool> itemsGrabbed_;

    AtomicPool(const AtomicPool& toCopy) = delete;
    AtomicPool& operator=(const AtomicPool& toCopy) = delete;

    /// Returns the index of the first ungrabbed item in the pool, atomically
    /// marking it as grabbed when it is found - or `INVALID_INDEX` if the pool
    /// is full (no ungrabbed items to be found in it).
    size_t grabIndex()
    {
        bool expectedGrabbed;

        for(size_t i = 0; i < n_; i ++)
        {
            expectedGrabbed = false; // The "grabbed?" value expected in the item to grab is `false`
            if(itemsGrabbed_[i].compare_exchange_weak(expectedGrabbed, true))
            {
                // Successfully grabbed what was an ungrabbed item
                // Note that now its "grabbed?" flag is true thanks to the atomic compare-exchange
                return i;
            }
        }

        // Found no ungrabbed item
        return INVALID_INDEX;
    }

    /// Returns `true` if the given item is in bounds for the current pool (i.e.
    /// it could possibly have been grabbed from it) or `false` otherwise.
    inline constexpr bool itemInBounds(const T* item) const
    {
        return item >= items_ && item < (items_ + n_);
    }

public:
    /// Creates a new, uninitialized (and invalid) pool.
    AtomicPool()
        : n_(0), items_(nullptr)
    {
    }

    /// Initializes a new atomic pool with capacity for `n` items.
    AtomicPool(size_t n)
        : n_(n), itemsGrabbed_(n)
    {
        items_ = new T[n_];

        // Mark all items as initially not grabbed
        for(size_t i = 0; i < n_; i ++)
        {
            itemsGrabbed_[i] = false;
        }
    }

    AtomicPool(AtomicPool&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    AtomicPool& operator=(AtomicPool&& toMove)
    {
        // Move data over
        n_ = toMove.n_;
        items_ = std::move(toMove.items_);
        itemsGrabbed_ = std::move(toMove.itemsGrabbed_);

        // Invalidate the moved instance
        toMove.items_ = nullptr;

        return *this;
    }

    ~AtomicPool()
    {
        delete[] items_; items_ = nullptr;
    }

    /// Returns `true` if the pool is currently valid (initialized, not destroyed,
    /// not moved) or `false` otherwise.
    operator bool() const
    {
        return items_;
    }

    /// Returns the number of items in the pool.
    inline size_t n() const
    {
        return n_;
    }


    /// Attempts a free item from the pool; returns it or null if no more free
    /// items are in the pool. This operation is atomic and thread-safe.
    /// **ASSERTS**: That the pool is currently valid
    T* grab()
    {
        assert(operator bool() && "Pool is invalid");

        size_t index = grabIndex();
        return index != INVALID_INDEX ? &items_[index] : nullptr;
    }

    /// Attempts to free (ungrab) item from the pool; returns it or null if no more free
    /// items are in the pool. This operation is atomic and thread-safe.
    /// **ASSERTS**: That the pool is currently valid and that the item could in fact
    ///              grabbed from the pool
    bool free(T* item)
    {
        assert(operator bool() && "Pool is invalid");
        assert(itemInBounds(item) && "Item is out of bounds");

        size_t itemIndex = item - items_;

        // The "grabbed?" value expected in the item to free is `true`
        bool expectedGrabbed = true;

        // Will return `true` if ungrabbing succeeded or `false` otherwise
        // (because the item was already ungrabbed)
        return itemsGrabbed_[itemIndex].compare_exchange_weak(expectedGrabbed, false);
    }

    /// **ASSERTS**: That the pool is currently valid and that the item could in fact
    ///              grabbed from the pool
    inline bool isGrabbed(const T* item) const
    {
        assert(operator bool() && "Pool is invalid");
        assert(itemInBounds(item) && "Item is out of bounds");

        size_t itemIndex = item - items_;
        return itemsGrabbed_[itemIndex];
    }
};

}
