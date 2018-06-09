#pragma once

#include <stddef.h>
#include <tuple>
#include <type_traits>
#include <algorithm>

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


/// Sorts a `begin...end` iterator according to the value that
/// `key(*it for it in begin..end)` assumes; in ascending order if `reverse` is
/// `false`, in descending order if it is `true`.
template <typename Iter, typename KeyFunc>
inline void sort(Iter begin, Iter end, KeyFunc key, bool reverse=false)
{
    using T = typename std::iterator_traits<Iter>::value_type;

    if(!reverse)
    {
        auto compFunc = [key](const T& a, const T& b)
        {
            // Low-to-high
            return key(a) < key(b);
        };
        std::sort(begin, end, compFunc);
    }
    else
    {
        auto compFunc = [key](const T& a, const T& b)
        {
            // High-to-low
            return key(a) >= key(b);
        };
        std::sort(begin, end, compFunc);
    }
}


/// A class whose `cbegin()` and `cend()` define an iterator range for `U` values,
/// each generated by calling `mapper(*it for it in tBegin...tEnd)`.
template <typename U, typename TIter, typename MapperFunc>
class IterMapper
{
    TIter tBegin_, tEnd_;
    MapperFunc mapperFunc_;

public:
    /// An input const iterator over the `U`s generated from the input `TIter`
    /// range.
    friend class const_iterator;
    class const_iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = U;
        using pointer = const value_type*;
        using reference = const value_type&;
        using difference_type = std::ptrdiff_t;

    private:
        friend class IterMapper;
        const IterMapper* parent_;
        TIter tIter_;
        U u_;

        const_iterator(const IterMapper* parent_, TIter tIter)
            : parent_(parent_), tIter_(tIter)
        {
            if(tIter_ != parent_->tEnd_)
            {
                // If this is an element in the `[begin..end)` range, lazily find the U now
                u_ = parent_->mapperFunc_(*tIter_);
            }
        }

    public:
        inline reference operator*() const
        {
            return u_;
        }


        inline const_iterator& operator++() // preincrement
        {
            tIter_ ++;
            if(tIter_ != parent_->tEnd_)
            {
                // If this is an element in the `[begin..end)` range, lazily find the U now
                u_ = parent_->mapperFunc_(*tIter_);
            }
            return *this;
        }

        inline const_iterator& operator++(int) // postincrement
        {
            const_iterator old = *this;
            (void)operator++();
            return old;
        }


        inline bool operator==(const const_iterator& other) const
        {
            // Note: value_ is implictly equal if `parent_` and `tIter_` match
            return parent_ == other.parent_ && tIter_ == other.tIter_;
        }

        inline bool operator!=(const const_iterator& other) const
        {
            // Note: value_ is implictly different if `parent_` and/or `tIter_`
            // are mismatched
            return parent_ != other.parent_ || tIter_ != other.tIter_;
        }
    };


    /// Instantiates a new IterMapper that generates `U`s for `T`s found in the
    /// iterator range `tBegin..tEnd` by using [a copy of] `mapperFunc`.
    ///
    /// `U`s are lazily generated progressively as `const_iterator`s are stepped.
    constexpr IterMapper(TIter tBegin, TIter tEnd, MapperFunc mapperFunc)
        : tBegin_(tBegin), tEnd_(tEnd), mapperFunc_(mapperFunc)
    {
    }

    constexpr IterMapper(const IterMapper& toCopy)
        : tBegin_(toCopy.tBegin_), tEnd_(toCopy.tEnd_), mapperFunc_(toCopy.mapperFunc_)
    {
    }
    IterMapper& operator=(const IterMapper& toCopy) = delete;

    constexpr IterMapper(IterMapper&& toMove)
        : tBegin_(std::move(toMove.tBegin_)), tEnd_(std::move(toMove.tEnd_)),
          mapperFunc_(std::move(toMove.mapperFunc_))
    {
    }
    IterMapper& operator=(IterMapper&& toMove) = delete;


    /// The begin `U` generator iterator.
    const_iterator cbegin() const
    {
        return {this, tBegin_};
    }

    /// The end `U` generator iterator.
    const_iterator cend() const
    {
        return {this, tEnd_};
    }
};


/// Maps the `begin..end` iterator range to a new `uBegin..uEnd` one, where each
/// of `uIt` in `uBegin..uEnd` correspoding to a `tIt` in `begin..end` is generated
/// by calling `func(*tIt)`
template <typename U, typename TIter, typename MapperFunc>
inline constexpr IterMapper<U, TIter, MapperFunc> map(TIter begin, TIter end, MapperFunc func)
{
    return IterMapper<U, TIter, MapperFunc>(begin, end, func);
}


}
