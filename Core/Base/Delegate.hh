#pragma once

// This file is based on code from http://blog.coldflake.com/posts/C++-delegates-on-steroids/,
// which is released at https://github.com/marcmo/delegates under the MIT license:
// =============================================================================
// The MIT License (MIT)
//
// Copyright (c) 2015 oliver.mueller@gmail.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// =============================================================================

#include <utility>
#include <Core/Api.h>

namespace Ares
{

template <typename Func>
class ARES_API Delegate;

/// A delegate for a function or method that is called with the arguments `Args`
/// and returns a value of type `Ret`.
///
/// This is a lightweight alternative to `std::function` **that does not own, but
/// only reference, callee and called function**.
template <typename Ret, typename... Args>
class ARES_API Delegate<Ret(Args...)>
{
    using Caller = Ret(*)(void*, Args&&...);

    void* obj_;
    Caller caller_;


    template <typename T, Ret(T::*method)(Args...)>
    inline static Ret methodCaller(void* obj, Args&&... args)
    {
        auto tPtr = static_cast<T*>(obj);
        return (tPtr->*method)(std::forward<Args>(args)...);
    }

    template <typename T, Ret(T::*const constMethod)(Args...) const>
    inline static Ret constMethodCaller(void* obj, Args&&... args)
    {
        auto tPtr = static_cast<const T*>(obj);
        return (tPtr->*constMethod)(std::forward<Args>(args)...);
    }

    inline static Ret functionCaller(void* obj, Args&&... args)
    {
        auto funcPtr = reinterpret_cast<Ret(*)(Args...)>(obj);
        return funcPtr(std::forward<Args>(args)...);
    }


    constexpr Delegate(void* obj, Caller caller)
        : obj_(obj), caller_(caller)
    {
    }

public:
    /// Creates a new null delegate (i.e. one that points to no function/method/functor).
    constexpr Delegate(std::nullptr_t null=nullptr)
        : obj_(nullptr), caller_(nullptr)
    {
    }

    /// Creates a delegate that will call `object->method()` when invoked.
    template <typename T, Ret(T::*method)(Args...)>
    inline static constexpr Delegate from(T* object)
    {
        return Delegate(object, &methodCaller<T, method>);
    }

    /// Creates a delegate that will call `object->method() const` when invoked.
    template <typename T, Ret(T::*constMethod)(Args...) const>
    inline static constexpr Delegate from(const T* object)
    {
        return Delegate(const_cast<T*>(object), &constMethodCaller<T, constMethod>);
    }

    /// Creates a delegate that will call `functor->operator()` when invoked.
    template <typename T>
    inline static constexpr Delegate from(T* functor)
    {
        return Delegate(functor, &methodCaller<T, &T::operator()>);
    }

    /// Creates a delegate that will call `functor->operator() const` when invoked.
    template <typename T>
    inline static constexpr Delegate from(const T* functor)
    {
        return Delegate(const_cast<T*>(functor), &constMethodCaller<T, &T::operator()>);
    }

    /// Creates a delegate that will call `function()` when invoked.
    inline static constexpr Delegate from(Ret(*function)(Args...))
    {
        // NOTE Casting from function pointer to `void*` is supported since C++11,
        //      and it was required for making `dlsym()` and friends work anyways
        return Delegate(reinterpret_cast<void*>(function), &functionCaller);
    }

    ~Delegate() = default;

    /// Returns `true` if the delegate is non-null.
    inline operator bool() const
    {
        return obj_ != nullptr;
        // `func_` may actually be `nullptr`; see `functionCaller()`
    }


    /// Invokes the delegate with the given arguments.
    /// **WARNING** Invoking a null delegate will cause a segmentation fault (or
    ///             an `assert()` in debug mode)!!
    inline Ret operator()(Args&&... args) const
    {
        assert(obj_ && "Caller is null!");
        return caller_(obj_, std::forward<Args>(args)...);
    }


    inline void operator==(const Delegate<Ret(Args...)>& other) const
    {
        return obj_ == other.obj_ && caller_ == other.caller_;
    }

    inline void operator!=(const Delegate<Ret(Args...)>& other) const
    {
        return obj_ != other.obj_ || caller_ != other.caller_;
    }
};


/// Returns a const pointer to `val`; convenience function equivalent to calling
/// `const_cast<const T*>(&val)`.
///
/// This is useful for constructing `Delegate`s that call a method marked `const`
/// in a class or that call a functor whose `operator()` is marked `const`, because
/// if a non-const pointer to the callee/functor is passed to `Delegate`'s constructor
/// `Delegate` will try to invoke a non-`const` version of the method - that
/// potentially does not exist!
template <typename T>
inline constexpr const T* constPtr(const T& val)
{
    return &val;
}

}
