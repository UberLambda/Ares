#include "Backend.hh"

#include <sstream>
#include "Shader.hh"
#include "Texture.hh"

namespace Ares
{
namespace GL33
{

Backend::Backend()
{
}

ErrString Backend::init()
{
    // Nothing to init here
    return {};
}

Backend::~Backend()
{
    // Destroy all leftover resources
    for(const auto& bufIt : buffers_)
    {
        delBuffer(bufIt.first);
    }
    for(const auto& texIt : textures_)
    {
        delTexture(texIt.first);
    }
    for(const auto& shdIt : shaders_)
    {
        delShader(shdIt);
    }
}


static constexpr const GLenum GFXUSAGE_TO_GL[] =
{
    GL_STATIC_DRAW, // 0: Static
    GL_DYNAMIC_DRAW, // 1: Dynamic
    GL_STREAM_DRAW, // 2: Streaming
};

Handle<GfxBuffer> Backend::genBuffer(const GfxBufferDesc& desc, ErrString* err)
{
    GLuint buffer = 0; glGenBuffers(1, &buffer);
    if(!buffer)
    {
        return {};
    }

    // TODO Check if a buffer can actually be rebound to a different target after
    //      it has been `glBufferData()`ed?
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 desc.size, desc.data,
                 GFXUSAGE_TO_GL[unsigned(desc.usage)]);

    Handle<GfxBuffer> handle(buffer);
    buffers_[handle] = desc;
    return handle;
}

void Backend::resizeBuffer(Handle<GfxBuffer> buffer, size_t newSize)
{
    auto descIt = buffers_.find(buffer);
    if(!buffer || descIt == buffers_.end())
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, newSize, nullptr,
                 GFXUSAGE_TO_GL[unsigned(descIt->second.usage)]);

    descIt->second.size = newSize;
}

void Backend::editBuffer(Handle<GfxBuffer> buffer, size_t dataOffset, size_t dataSize, const void* data)
{
    auto descIt = buffers_.find(buffer);
    if(!buffer || descIt == buffers_.end()
       || descIt->second.size < (dataOffset + dataSize)) // (New data would not fit this buffer)
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferSubData(GL_ARRAY_BUFFER, dataOffset, dataSize, data);
}

void Backend::delBuffer(Handle<GfxBuffer> buffer)
{
    auto descIt = buffers_.find(buffer);
    if(!buffer || descIt == buffers_.end())
    {
        return;
    }

    GLuint bufferName = descIt->first;
    glDeleteBuffers(1, &bufferName);

    buffers_.erase(descIt);
}


static constexpr const GLenum GFX_TEXTURE_DATATYPE_TO_GL[] = // (index by `GfxTextureDesc::DataType`)
{
    GL_UNSIGNED_BYTE, // U8
    GL_UNSIGNED_SHORT, // U16
    GL_FLOAT, // F32
};

static constexpr const GLenum GFX_TEXTURE_MIN_FILTER_TO_GL[] = // (index by `GfxTextureDesc::Filter`)
{
    GL_NEAREST, // Nearest
    GL_LINEAR,  // Bilinear
    GL_LINEAR_MIPMAP_LINEAR, // Trilinear
    GL_LINEAR_MIPMAP_LINEAR, // Anisotropic (aka trilinear + anisotropic)
};

static constexpr const GLenum GFX_TEXTURE_MAG_FILTER_TO_GL[] = // (index by `GfxTextureDesc::Filter`)
{
    GL_NEAREST, // Nearest
    GL_LINEAR,  // Bilinear
    GL_LINEAR, // Trilinear
    GL_LINEAR, // Anisotropic (aka trilinear + anisotropic)
};

Handle<GfxTexture> Backend::genTexture(const GfxTextureDesc& desc, ErrString* err)
{
    GLuint texture = 0; glGenTextures(1, &texture);
    if(!texture)
    {
        return {};
    }

    GLenum format, internalFormat;
    if(!textureFormats(format, internalFormat, desc.format))
    {
        // Found no suitable OpenGL texture format for the supplied ImageFormat
        return {};
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 internalFormat,
                 desc.resolution.width, desc.resolution.height,
                 0,
                 format,
                 GFX_TEXTURE_DATATYPE_TO_GL[unsigned(desc.dataType)], desc.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GFX_TEXTURE_MIN_FILTER_TO_GL[unsigned(desc.minFilter)]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GFX_TEXTURE_MAG_FILTER_TO_GL[unsigned(desc.magFilter)]);

    // FIXME IMPORTANT **Load OpenGL anisotropic exension and apply anisotropy
    //                   at a Renderer-decided level to this texture if minFilter=Anisotropic**

    // TODO PERFORMANCE Generate mipmaps only as needed, maybe on another thread?
    glGenerateMipmap(GL_TEXTURE_2D);

    Handle<GfxTexture> handle(texture);
    textures_[handle] = {desc, format, internalFormat};
    return handle;
}

void Backend::resizeTexture(Handle<GfxTexture> texture, Resolution newResolution)
{
    auto descIt = textures_.find(texture);
    if(!texture || descIt == textures_.end())
    {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 descIt->second.internalFormat,
                 newResolution.width, newResolution.height,
                 0,
                 descIt->second.format,
                 GL_FLOAT, nullptr);

    descIt->second.desc.resolution = newResolution;
}

void Backend::editTexture(Handle<GfxTexture> texture, ViewRect dataRect, const void* data)
{
    auto descIt = textures_.find(texture);
    if(!texture || descIt == textures_.end())
    {
        return;
    }

    GLuint xOffset = dataRect.bottomLeft.x;
    GLuint yOffset = dataRect.topRight.y;
    GLuint width = dataRect.topRight.x - xOffset;
    GLuint height = yOffset - dataRect.bottomLeft.y;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    xOffset, yOffset, width, height,
                    descIt->second.format,
                    GFX_TEXTURE_DATATYPE_TO_GL[unsigned(descIt->second.desc.dataType)],
                    data);
}

void Backend::delTexture(Handle<GfxTexture> texture)
{
    auto descIt = textures_.find(texture);
    if(!texture || descIt == textures_.end())
    {
        return;
    }

    GLuint textureName = descIt->first;
    glDeleteTextures(1, &textureName);

    textures_.erase(descIt);
}


struct GlShaderDesc
{
    const char* name;
    GLenum type;
    const char* source;
};

Handle<GfxShader> Backend::genShader(const GfxShaderDesc& desc, ErrString* err)
{
    GlShaderDesc oglDescs[] =
    {
        {"vertex", GL_VERTEX_SHADER, desc.src.vert.c_str()},
        {"fragment", GL_FRAGMENT_SHADER, desc.src.frag.c_str()},
        {"geometry", GL_GEOMETRY_SHADER, desc.src.geom.c_str()},
        // FIXME: Also implement `src.tes`, `src.tcs` if have appropriate EXTs
    };

    GLuint oglShaders[5] = {0, 0, 0, 0, 0}; // vert, frag, geom, tes, tcs
    GLuint oglProgram = 0;
    ErrString oglErr;

    for(int i = 0; i < sizeof(oglDescs) / sizeof(GlShaderDesc); i ++)
    {
        const GlShaderDesc& oglDesc = oglDescs[i];
        oglErr = compileShader(oglShaders[i], oglDesc.type, oglDesc.source);
        if(oglErr)
        {
            if(*err)
            {
                std::ostringstream errMsg;
                errMsg << "Failed to compile " << oglDesc.name << " shader:\n" << oglErr;
                *err = errMsg.str();
            }
            return {};
        }
    }

    oglErr = linkShaderProgram(oglProgram,
                               oglShaders, oglShaders + sizeof(oglShaders) / sizeof(GLuint));
    if(oglErr)
    {
        if(*err)
        {
            std::ostringstream errMsg;
            errMsg << "Failed to link shader program:\n" << oglErr;
            *err = errMsg.str();
        }
        return {};
    }

    shaders_.emplace(oglProgram);
    return Handle<GfxShader>(oglProgram);
}

void Backend::delShader(Handle<GfxShader> shader)
{
    auto it = shaders_.find(shader);
    if(shader != 0 && it != shaders_.end())
    {
        glDeleteProgram(*it);
        shaders_.erase(it);
    }
}


void Backend::changeResolution(Resolution resolution)
{
    glViewport(0, 0, resolution.width, resolution.height);
}

void Backend::runCmds(const GfxCmd* cmds, const GfxCmdIndex* cmdsOrder, size_t n)
{
    for(size_t i = 0; i < n; i ++)
    {
        const GfxCmd& cmd = cmds[cmdsOrder[i].index];
        // FIXME IMPLEMENT!
    }
}

}
}
