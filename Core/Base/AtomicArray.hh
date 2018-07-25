#pragma once

#include <stddef.h>
#include <atomic>
#include <utility>
#include <Core/Api.h>

namespace Ares
{

/// An heap-allocated array of atomic `T`s.
template <typename T>
class ARES_API AtomicArray
{
public:
    /// Each item of the array, that contains an atomic `T`.
    using Item = std::atomic<T>;

private:
    size_t n_;
    Item* items_;

    AtomicArray(const AtomicArray& toCopy) = delete;
    AtomicArray& operator=(const AtomicArray& toCopy) = delete;

public:
    /// Creates an uninitialized (invalid) array.
    AtomicArray()
        : n_(0), items_(nullptr)
    {
    }

    /// Initializes an array with `n` items in it.
    AtomicArray(size_t n)
        : n_(n)
    {
        // TODO Assert that `n != 0`?
        items_ = new Item[n];
    }

    AtomicArray(AtomicArray&& toMove)
    {
        (void)operator=(std::move(toMove));
    }
    AtomicArray& operator=(AtomicArray&& toMove)
    {
        // Move data over
        items_ = toMove.items_;
        n_ = toMove.n_;

        // Invalidate the moved instance
        toMove.items_ = nullptr;

        return *this;
    }

    /// Frees the array.
    ~AtomicArray()
    {
        delete[] items_; items_ = nullptr;
    }

    /// Returns `true` if the array is valid (not moved, not destroyed) or `false`
    /// otherwise.
    inline operator bool() const
    {
        return items_ != nullptr;
    }


    /// Returns the number of items in the array.
    inline size_t n() const
    {
        return n_;
    }

    inline const Item& operator[](size_t index) const
    {
        // TODO Bounds check assertion?
        return items_[index];
    }
    inline Item& operator[](size_t index)
    {
        // TODO Bounds check assertion?
        return items_[index];
    }
};

}
