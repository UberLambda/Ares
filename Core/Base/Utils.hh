#pragma once

#include <stddef.h>
#include <tuple>

namespace Ares
{

/// Returns the minimum between two `T`s.
/// If `a` and `b` are equal, returns `b`.
template <typename T>
inline T min(T a, T b)
{
    return a < b ? a : b;
}

/// Returns the maximum between two `T`s.
/// If `a` and `b` are equal, returns `b`.
template <typename T>
inline T max(T a, T b)
{
    return a > b ? a : b;
}


/// Returns true if any of `*it` evaluate to true, for `it` in `begin..end`.
/// Lazy: stops iterating on the first "truthy" iterator found.
template <typename Iter>
inline bool any(Iter begin, Iter end)
{
    for(Iter it = begin; it != end; it ++)
    {
        if(*it)
        {
            return true;
        }
    }
    return false;
}

/// Returns true if any of `func(*it)` evaluates to true, for `it` in `begin..end`.
/// Lazy: stops iterating on the first "truthy" iterator found.
template <typename Iter, typename Func>
inline bool any(Iter begin, Iter end, Func func)
{
    for(Iter it = begin; it != end; it ++)
    {
        if(func(*it))
        {
            return true;
        }
    }
    return false;
}

/// Returns true if all of `*it` evaluate to true, for `it` in `begin..end`.
/// Lazy: stops iterating on the first "falsey" iterator found.
template <typename Iter>
inline bool all(Iter begin, Iter end)
{
    for(Iter it = begin; it != end; it ++)
    {
        if(!(*it))
        {
            return false;
        }
    }
    return true;
}

/// Returns true if all of `func(*it)` evaluate to true, for `it` in `begin..end`.
/// Lazy: stops iterating on the first "truthy" iterator found.
template <typename Iter, typename Func>
inline bool all(Iter begin, Iter end, Func func)
{
    for(Iter it = begin; it != end; it ++)
    {
        if(!func(*it))
        {
            return false;
        }
    }
    return true;
}

}
