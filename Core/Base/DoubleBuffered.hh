#pragma once

#include <atomic>
#include <utility>
#include <Core/Api.h>

namespace Ares
{

/// A holder for two `T`s that can be swapped to pingpong (doublebuffer)
/// between them.
template <typename T>
class ARES_API DoubleBuffered
{
    T items_[2];
    int which_;

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
        which_ = toMove.which_;

        // Moved instance is invalidated by having its `T`s moved!

        return *this;
    }

    ~DoubleBuffered() = default;


    /// Returns a reference to the item of the two that is "current".
    inline T& current()
    {
        return items_[which_];
    }

    /// Returns a reference to the item of the two that is "past".
    inline T& past()
    {
        return items_[!which_];
    }

    /// Swaps `current()` and `swap()`.
    /// **NOT** threadsafe!
    inline void swap()
    {
        which_ ^= 1;
    }
};

}
