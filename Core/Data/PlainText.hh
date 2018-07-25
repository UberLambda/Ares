#pragma once

#include <string>
#include <Core/Api.h>
#include <Core/Data/ResourceParser.hh>

namespace Ares
{

/// A plain text resource.
using PlainText = std::string;


template <>
struct ARES_API ResourceParser<PlainText>
{
    static ErrString parse(PlainText& outResource, std::istream& stream, const char* ext,
                           ResourceLoader& loader)
    {
        stream.seekg(0, std::ios::end);
        outResource.resize(stream.tellg());
        stream.seekg(0, std::ios::beg);

        stream.read(&outResource[0], outResource.length());

        return {}; // (parsing a plain text file never fails)
    }
};

}
