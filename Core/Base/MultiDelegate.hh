#pragma once

#include <vector>
#include <utility>
#include <Core/Api.h>
#include <Core/Base/Delegate.hh>

namespace Ares
{

template <typename Func>
class ARES_API MultiDelegate;

/// A group of `Delegate<void(Args...)>`s that all get invoked when the `MultiDelegate`
/// is invoked.
template <typename... Args>
class ARES_API MultiDelegate<void(Args...)>
{
public:
    /// The corresponding `Delegate` type to this `MultiDelegate` one.
    using Delegate = Ares::Delegate<void(Args...)>;

private:
    std::vector<Delegate> delegates_;

public:
    MultiDelegate() = default;
    ~MultiDelegate() = default;


    /// Appends the given delegate to the list of delegates to invoke.
    /// Note that more than one copy of the same delegate can be added to the
    /// `MultiDelegate`; in that case, the delegate will be invoked more than
    /// once.
    inline MultiDelegate& operator+=(const Delegate& delegate)
    {
        delegates_.push_back(delegate);
        return *this;
    }

    /// Removes all copies of the given delegate from the list of the delegates
    /// to invoke.
    inline MultiDelegate& operator-=(const Delegate& delegate)
    {
        delegates_.erase(delegate);
        return *this;
    }

    /// Returns the length of the list of delegates to invoke.
    inline size_t nDelegates() const
    {
        return delegates_.size();
    }

    /// Clears all delegates from the list of delegates to invoke.
    void clear()
    {
        delegates_.clear();
    }


    /// Invokes all delegates added to this `MultiDelegate`.
    inline void operator()(Args&&... args)
    {
        for(auto& delegate : delegates_)
        {
            delegate(std::forward<Args>(args)...);
        }
    }
};

}
