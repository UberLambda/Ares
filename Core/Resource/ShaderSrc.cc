#include "ShaderSrc.hh"

#include <sstream>
#include "Config.hh"
#include "../Data/ResourceLoader.hh"

namespace Ares
{

/// See `readCfgStringValue()`
enum StringRead
{
    Read,
    Missing,
    TypeError,
};

/// Attempts to read `cfg[key].value.string`:
/// - If the string value is found and it is a string, sets `outValue` and returns `Read`.
/// - If the string value is not found, leaves `outValue` unchanged and returns `Missing`.
/// - If a value is found but it is not a string, leaves `outValue` unchanged and returns `TypeError`.
static StringRead readCfgStringValue(std::string& outValue,
                                     const Config& cfg, const ConfigKey& key)
{
    ConfigValue noValue;
    noValue.type = ConfigValue::I64;
    noValue.value.i64 = 42;

    ConfigValue value = cfg.get(key, noValue);
    if(value == noValue)
    {
        return Missing;
    }
    else if(value.type != ConfigValue::String)
    {
        return TypeError;
    }
    else
    {
        outValue = value.value.string; // Copy string
        return Read;
    }
}

/// Attempts to read a shader soruce for a shader of type `type`; the shader source
/// will either be read from config["${name}.src"], or from an external file at
/// config["${name}"].
static ErrString readShaderConfigKey(std::string& outSrc, const char* type,
                                     const Config& shaderCfg, const Path& shaderPath,
                                     ResourceLoader& loader)
{
    ConfigKey filepathKey("shader."); // Cfg key containing a path to an external file
    filepathKey += type;

    ConfigKey srcKey(filepathKey); // Cfg key containing the shader source directly
    srcKey += ".src";

    std::string filepath;
    auto filepathRead = readCfgStringValue(filepath, shaderCfg, filepathKey);
    if(filepathRead == TypeError)
    {
        return ErrString(filepathKey) + " has the wrong type";
    }

    auto srcRead = readCfgStringValue(outSrc, shaderCfg, srcKey);
    if(srcRead == TypeError)
    {
        return ErrString(std::move(srcKey)) + " has the wrong type";
    }

    if(filepathRead == Missing && srcRead == Missing)
    {
        // Shader not found, but it's not an error; just leave `outSrc` empty
        return {};
    }
    else if(filepathRead == Read && srcRead == Read)
    {
        std::ostringstream err;
        err << "Both " << std::move(srcKey) << " and " << std::move(filepathKey) << " defined";
        return ErrString(std::move(err.str()));
    }
    else if(filepathRead == Read)
    {
        if(filepath.length() >= 1 && filepath[1] == '/')
        {
            // Absolute path, keep it as-is
        }
        else
        {
            // Relative path to the shader file
            std::ostringstream absPath;

            Path dir = shaderPath.dirname();
            absPath << dir;
            if(!dir.str().empty())
            {
                // Insert separator between dir and name
                absPath << '/';
            }
            absPath << filepath;

            filepath = std::move(absPath.str());
        }

        // Shader is in external file
        std::istream* stream = loader.fileStore()->getStream(filepath);
        if(!stream)
        {
            std::ostringstream err;
            err << "Source file for " << type << " shader not found";
            return std::move(err.str());
        }

        stream->seekg(0, std::ios::end);
        size_t fileSize = stream->tellg();
        stream->seekg(0, std::ios::beg);

        // Note: one can safely writte over the null terminator in a `std::string`
        outSrc.resize(fileSize - 1);
        stream->read(&outSrc[0], fileSize);

        bool readOk = stream->operator bool();
        loader.fileStore()->freeStream(stream);

        if(!readOk)
        {
            std::ostringstream err;
            err << "Source file for " << type << " shader: read error";
            return std::move(err.str());
        }
    }

    // Success; read to `outSrc`
    return {};
}


struct SourcePair
{
    const char* type;
    std::string& source;
};

ErrString ResourceParser<ShaderSrc>::parse(ShaderSrc& outSrc, std::istream& stream,
                                           const Path& path, ResourceLoader& loader)
{
    Config shaderCfg;
    auto cfgErr = ResourceParser<Config>::parse(shaderCfg, stream, path, loader);
    if(cfgErr)
    {
        return ErrString("Error while parsing shader source: ") + cfgErr;
    }

    SourcePair shaderStrPairs[] = // shader type name => source string to read
    {
        {"vert", outSrc.vert},
        {"frag", outSrc.frag},
        {"geom", outSrc.geom},
        {"tcs", outSrc.tcs},
        {"tes", outSrc.tes},
    };

    unsigned int nSourcesRead = 0;
    ErrString err;
    for(SourcePair pair : shaderStrPairs)
    {
        err = readShaderConfigKey(pair.source, pair.type,
                                  shaderCfg, path, loader);
        if(err)
        {
            return err;
        }

        if(!pair.source.empty())
        {
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
