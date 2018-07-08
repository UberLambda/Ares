#pragma once

#include <string>
#include "ResourceParser.hh"

namespace Ares
{

/// A plain text resource.
using PlainText = std::string;


template <>
struct ResourceParser<PlainText>
{
    static ErrString parse(PlainText& outResource, std::istream& stream, const char* ext)
    {
        stream.seekg(0, std::ios::end);
        outResource.resize(stream.tellg());
        stream.seekg(0, std::ios::beg);

        stream.read(&outResource[0], outResource.length());

        return {}; // (parsing a plain text file never fails)
    }
};

}
