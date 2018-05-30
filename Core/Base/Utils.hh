#pragma once

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
T max(T a, T b)
{
    return a > b ? a : b;
}

}
