#pragma once

#include "../Data/ResourceParser.hh"
#include <json.hpp>

namespace Ares
{

/// The representation of a Json-like file.
using Json = nlohmann::json;

/// Implementation of `ResourceParser` for `Json`s.
template <>
struct ResourceParser<Json>
{
    static ErrString parse(Json& outJson, std::istream& stream,
                           const Path& path, ResourceLoader& loader);
};

}
