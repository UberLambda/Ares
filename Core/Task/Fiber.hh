#pragma once

#include <stddef.h>
#include <utility>
#include <boost_context/fcontext.h>
#include <Core/Base/NumTypes.hh>

// Based on the fiber implementation in https://github.com/RichieSams/FiberTaskingLib (Apache2 license)
// License of FiberTaskingLib is contained in "3rdparty/boost_context_ftl/LICENSE.md"

namespace Ares
{

/// A function to be run in a `Fiber`.
using FiberFunc = void(*)(void* data);

/// A fiber. See: https://en.wikipedia.org/wiki/Fiber_(computer_science)
class Fiber
{
    void* stack_;
    size_t stackSize_;
    boost_context::fcontext_t context_;
    void* data_;

    Fiber(const Fiber& toCopy) = delete;
    Fiber& operator=(const Fiber& toCopy) = delete;

public:
    /// Creates an uninitialized fiber. The fiber is to be initialized later by
    /// `std::moving` an initialized one into it.
    Fiber()
        : stack_(nullptr), stackSize_(0), context_(nullptr), data_(nullptr)
    {
    }

    /// Initializes a fiber that will run `func` when switched to given its stack,
    /// its stack's size in bytes, and some data that will be passed to `func`.
    /// `std::moving` an initialized one into it.
    Fiber(FiberFunc func, void* stack, size_t stackSize, void* data=nullptr)
        : stack_(stack), stackSize_(stackSize), data_(data)
    {
        // TODO Assert that `func != nullptr` && `stack != nullptr` && `stackSize != 0` here
        auto sp = reinterpret_cast<U8*>(stack) + stackSize_; // (See boost_context_ftl)
        context_ = boost_context::make_fcontext((void*)sp, stackSize, func);
    }

    Fiber(Fiber&& toMove)
        : Fiber()
    {
        (void)operator=(std::move(toMove));
    }

    Fiber& operator=(Fiber&& toMove)
    {
        // Move data over
        stack_ = toMove.stack_;
        stackSize_ = toMove.stackSize_;
        context_ = std::move(toMove.context_);
        data_ = toMove.data_;

        // Invalidate the moved instance
        toMove.stack_ = nullptr;
        toMove.stackSize_ = 0;
        toMove.context_ = nullptr;
        toMove.data_ = nullptr;

        return *this;
    }

    /// Returns `true` if the fiber is initialized or `false` otherwise.
    /// Returns `false` for moved and/or destroyed fibers.
    inline operator bool() const
    {
        return stack_ && stackSize_ != 0 && context_;
    }

    ~Fiber()
    {
        stack_ = nullptr;
        stackSize_ = 0;
        context_ = nullptr;
    }


    /// Stops the current fiber and starts `otherFiber`; after `otherFiber` is
    /// done running, resumes the current one.
    inline void switchTo(Fiber& otherFiber)
    {
        boost_context::jump_fcontext(&this->context_, otherFiber.context_, otherFiber.data_);
    }


    /// Returns a pointer to the fiber's stack. Will return null if uninitialized.
    inline void* stack()
    {
        return stack_;
    }

    /// Returns the size of the fiber's stack. Will return 0 if uninitialized.
    inline size_t stackSize() const
    {
        return stackSize_;
    }

    /// Returns the data that is to be passed to the fiber's function. Will return
    /// null if uninitialized.
    inline void* data()
    {
        return data_;
    }
};

}
