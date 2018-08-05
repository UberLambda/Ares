#pragma once

#include <streambuf>
#include <Core/Api.h>
#include <Core/Base/Utils.hh>

namespace Ares
{

/// A streambuf that does nothing of its input data.
class ARES_API NullStreambuf : public std::streambuf
{
public:
    NullStreambuf() = default;
    ~NullStreambuf() = default;

protected:
    int_type overflow(int_type c=EOF) override
    {
        return traits_type::to_int_type(c);
    }

    std::streamsize xsputn(const char_type* s, std::streamsize n) override
    {
        return n;
    }
};

}
