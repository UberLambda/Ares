#pragma once

#include <string.h>
#include <stdlib.h>
#include <streambuf>
#include <Core/Base/Utils.hh>

namespace Ares
{

/// A streambuf over a preallocated memory block.
class MemStreambuf : public std::streambuf
{
    MemStreambuf(const MemStreambuf& toCopy) = delete;
    MemStreambuf& operator=(const MemStreambuf& toCopy) = delete;

    char* mem_;
    std::streamsize size_;
    std::streamsize used_;

public:
    /// Creates a new, uninitialized memory streambuf.
    MemStreambuf()
        : mem_(nullptr), size_(0), used_(0)
    {
    }

    /// Initializes a new memory streambuf over the given memory block.
    MemStreambuf(void* mem, std::streamsize size)
        : mem_(reinterpret_cast<char*>(mem)), size_(size), used_(0)
    {
    }

    MemStreambuf(MemStreambuf&& toMove)
    {
        (void)operator=(std::move(toMove));
    }

    MemStreambuf& operator=(MemStreambuf&& toMove)
    {
        // Move data over
        mem_ = toMove.mem_;
        size_ = toMove.size_;
        used_ = toMove.used_;

        // Invalidate moved instance
        toMove.mem_ = nullptr;
        toMove.size_ = 0;

        return *this;
    }

    /// Returns `true` if the `MemStreambuf` is valid (initialized with a valid
    /// memory block, not moved) or `false` otherwise.
    inline operator bool() const
    {
        return mem_;
    }


    /// Returns the number of bytes used out of `size()`.
    inline std::streamsize used() const
    {
        return used_;
    }

    /// Returns the size of the underyling memory block in bytes.
    /// Returns zero if uninitialized/moved.
    inline std::streamsize size() const
    {
        return size_;
    }

protected:
    int_type overflow(int_type c=EOF) override
    {
        if(!mem_ || used_ == size_)
        {
            // Not initialized or not enough space left
            return EOF;
        }

        mem_[used_] = char(c);
        used_ ++;
        return traits_type::to_int_type(c);
    }

    std::streamsize xsputn(const char_type* s, std::streamsize n) override
    {
        if(!mem_)
        {
            // Not initialized
            return 0;
        }

        auto left = size_ - used_;
        auto copied = min(left, n);
        memcpy(mem_ + used_, s, copied);
        used_ += copied;
        return copied;
    }

public:
    /// Clears the contents of the buffer. Note that this does not actually clear
    /// the contents, but just put `used()` back to 0.
    void clear()
    {
        used_ = 0;
    }
};

}
