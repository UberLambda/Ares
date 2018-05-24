#pragma once

#include <stddef.h>
#include <utility>
#include "../Base/NumTypes.hh"

namespace Ares
{

/// A data store for fiber stacks.
class FiberStackStore
{
    size_t n_;
    size_t stackSize_;
    U8* stacks_;

    FiberStackStore(const FiberStackStore& toCopy) = delete;
    FiberStackStore& operator=(const FiberStackStore& toCopy) = delete;

public:
    /// Creates an uninitialized (invalid) stack store.
    FiberStackStore()
        : n_(0), stackSize_(0), stacks_(nullptr)
    {
    }

    /// Initalizes a stack store that can hold `n` stacks of `stackSize` bytes each.
    FiberStackStore(size_t n, size_t stackSize)
        : n_(n), stackSize_(stackSize)
    {
        stacks_ = new U8[n_ * stackSize_];
    }

    FiberStackStore(FiberStackStore&& toMove)
        : FiberStackStore()
    {
        (void)operator=(std::move(toMove));
    }

    FiberStackStore& operator=(FiberStackStore&& toMove)
    {
        // Move data over
        n_ = toMove.n_;
        stackSize_ = toMove.stackSize_;
        stacks_ = std::move(toMove.stacks_);

        // Invalidate the moved instance
        toMove.stacks_ = nullptr;

        return *this;
    }

    ~FiberStackStore()
    {
        delete[] stacks_; stacks_ = nullptr;
    }

    /// Returns `true` if the stack store is valid (initialized, not moved, not
    /// destroyed) or `false` otherwise.
    inline operator bool() const
    {
        return stacks_ != nullptr;
    }


    /// Returns the stack stored for the `index`th fiber.
    /// **ASSERTS**: `operator bool()`
    inline U8* operator[](size_t index)
    {
        assert(operator bool() && "Invalid stack store");
        return stacks_ + (index * stackSize_);
    }


    /// Returns the number of stacks in the store. Will return 0 for uninitialized stores.
    inline size_t n() const
    {
        return n_;
    }

    /// Returns the size of each stack in the store. Will return 0 for uninitialized stores.
    inline size_t stackSize() const
    {
        return stackSize_;
    }
};

}
