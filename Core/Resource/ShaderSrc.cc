#include "ShaderSrc.hh"

#include <sstream>
#include "Json.hh"
#include "../Data/ResourceLoader.hh"

namespace Ares
{

static ErrString readResourceFile(std::string& output, const Path& resPath,
                                  ResourceLoader& loader)
{
    std::istream* stream = loader.fileStore()->getStream(resPath);
    if(!stream)
    {
        return ErrString("Failed to open file");
    }

    stream->seekg(0, std::ios::end);
    size_t fileSize = stream->tellg();
    stream->seekg(0, std::ios::beg);
    output.resize(fileSize - 1);
    stream->read(&output[0], fileSize); // (will overwrite the internal trailing '\0')

    bool ok = stream->operator bool();

    loader.fileStore()->freeStream(stream);

    if(ok)
    {
        return {};
    }
    else
    {
        return ErrString("Error during file read");
    }
}

struct SourcePair
{
    const char* type;
    std::string* source;
};

ErrString ResourceParser<ShaderSrc>::parse(ShaderSrc& outSrc, std::istream& stream,
                                           const Path& path, ResourceLoader& loader)
{
    Json shaderJson;
    auto jsonErr = ResourceParser<Json>::parse(shaderJson, stream, path, loader);
    if(jsonErr)
    {
        return ErrString("Error while parsing shader: ") + jsonErr;
    }

    SourcePair shaderStrPairs[] = // shader type name => source string to read
    {
        {"vert", &outSrc.vert},
        {"frag", &outSrc.frag},
        {"geom", &outSrc.geom},
        {"tcs", &outSrc.tcs},
        {"tes", &outSrc.tes},
    };

    unsigned int nSourcesRead = 0;
    for(SourcePair pair : shaderStrPairs)
    {
        auto val = shaderJson[pair.type];
        if(val.is_null())
        {
            continue;
        }
        else if(!val.is_string())
        {
            std::ostringstream err;
            err << "Expected " << pair.type
                << " to be a resource path, but found a " << val.type_name();
            return ErrString(std::move(err.str()));
        }
        else
        {
            // Try to load source code from external file
            Path sourceFilePath = val;
            if(sourceFilePath.str().empty())
            {
                std::ostringstream err;
                err << "Resource path for " << pair.type << " is empty";
                return ErrString(std::move(err.str()));
            }

            if(sourceFilePath.str()[0] != '/')
            {
                // Path is relative to the location of the .arsh file
                std::ostringstream absPath;
                absPath << path.dirname() << '/' << sourceFilePath;
                sourceFilePath = std::move(absPath.str());
            }

            ErrString readErr = readResourceFile(*pair.source, sourceFilePath, loader);
            if(readErr)
            {
                std::ostringstream err;
                err << "Could not read source for " << pair.type << ": " << readErr;
                return ErrString(std::move(err.str()));
            }
            nSourcesRead ++;
        }
    }

    if(nSourcesRead == 0)
    {
        return "No shader sources defined";
    }
    else
    {
        // Success
        return {};
    }
}

}
