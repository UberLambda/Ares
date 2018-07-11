#include "Json.hh"

namespace Ares
{

ErrString ResourceParser<Json>::parse(Json& outJson, std::istream& stream,
                                      const Path& path, ResourceLoader& loader)
{
    try
    {
        stream >> outJson;
    }
    catch(Json::parse_error& err)
    {
        return ErrString(err.what());
    }

    return {};
}

}
