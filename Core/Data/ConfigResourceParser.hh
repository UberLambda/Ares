#pragma once

#include "../Data/ResourceParser.hh"
#include "Config.hh"

namespace Ares
{

/// Implementation of `ResourceParser` for `Config`s.
template <>
struct ResourceParser<Config>
{
    static ErrString parse(Config& outCfg, std::istream& stream, const char* ext,
                           ResourceLoader& loader);
};

}
