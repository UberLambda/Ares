#pragma once

#include <atomic>
#include <utility>

namespace Ares
{

/// A holder for two `T`s that can be swapped to pingpong (doublebuffer)
/// between them.
template <typename T>
class DoubleBuffered
{
    T items_[2];
    std::atomic<int> which_;

    DoubleBuffered(const DoubleBuffered& toCopy) = delete;
    DoubleBuffered& operator=(const DoubleBuffered& toCopy) = delete;

public:
    /// Initializes a new doublebuffer of two default-constructed `T`s.
    DoubleBuffered()
        : items_(), which_(0)
    {
    }

    DoubleBuffered(DoubleBuffered&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    DoubleBuffered& operator=(DoubleBuffered&& toMove)
    {
        // Move data over
        items_[0] = std::move(toMove.items_[0]);
        items_[1] = std::move(toMove.items_[1]);
        which_ = toMove.which_.load(); // (atomic load + store)
        
        // Moved instance is invalidated by having its `T`s moved!

        return *this;
    }

    ~DoubleBuffered() = default;


    /// Returns a pointer to the item of the two that is "current".
    /// Threadsafe and lockless.
    inline T* current()
    {
        return items_[which_.load()];
    }

    /// Returns a pointer to the item of the two that is "past".
    /// Threadsafe and lockless.
    inline T* past()
    {
        return items_[!which_.load()];
    }

    /// Swaps `current()` and `swap()`.
    /// Threadsafe and lockless.
    inline void swap()
    {
        which_ ^= 1; // (atomic fetch/xor)
    }
}

}
