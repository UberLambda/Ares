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

ErrString Backend::init(Ref<GfxPipeline> pipeline)
{
    // This could be the first time `init()` is called, or a 2+ time in case the
    // user wants to change the current pipeline.
    // Do *NOT* call ~Backend()` here, it would destroy the textures created by
    // the user to use as targets for the pipeline!

    // Set pipeline now so that `createPassFbo()` and other functions can access it
    pipeline_ = pipeline;

    // Generate `passData_` based on the passes in the pipeline
    size_t nPasses = pipeline->passes.size();
    passData_.resize(nPasses);

    for(size_t i = 0; i < nPasses; i ++)
    {
        const GfxPipeline::Pass& pass = pipeline->passes[i];
        passData_[i].program = pass.shader;
        passData_[i].ubo = pass.uniformBuffer;

        ErrString fboErr = createPassFbo(i);
        if(fboErr)
        {
            return fboErr;
        }
    }

    return {};
}

Backend::~Backend()
{
    // v----- Destroy all leftover resources -----v

    for(auto& passData : passData_)
    {
        glDeleteFramebuffers(1, &passData.fbo); passData.fbo = 0;
    }
    passData_.clear();

    for(const auto& bufIt : buffers_)
    {
        GLuint buffer = bufIt.first;
        glDeleteBuffers(1, &buffer);
    }
    buffers_.clear();

    for(const auto& texIt : textures_)
    {
        GLuint texture = texIt.first;
        glDeleteTextures(1, &texture);
    }
    textures_.clear();

    for(const auto& shdIt : shaders_)
    {
        GLuint program = shdIt;
        glDeleteProgram(program);
    }
    shaders_.clear();
}


static constexpr const GLenum GFXUSAGE_TO_GL[] =
{
    GL_STATIC_DRAW, // 0: Static
    GL_DYNAMIC_DRAW, // 1: Dynamic
    GL_STREAM_DRAW, // 2: Streaming
};

Handle<GfxBuffer> Backend::genBuffer(const GfxBufferDesc& desc)
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

Handle<GfxTexture> Backend::genTexture(const GfxTextureDesc& desc)
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

    if(desc.minFilter != GfxTextureDesc::Nearest
       && desc.minFilter != GfxTextureDesc::Bilinear)
    {
        // Generate mipmaps only if required
        glGenerateMipmap(GL_TEXTURE_2D);
    }

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
    const GfxTextureDesc& desc = descIt->second.desc;

    GLuint xOffset = dataRect.bottomLeft.x;
    GLuint yOffset = dataRect.topRight.y;
    GLuint width = dataRect.topRight.x - xOffset;
    GLuint height = yOffset - dataRect.bottomLeft.y;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    xOffset, yOffset, width, height,
                    desc.format,
                    GFX_TEXTURE_DATATYPE_TO_GL[unsigned(desc.dataType)],
                    data);

    if(desc.minFilter != GfxTextureDesc::Nearest
       && desc.minFilter != GfxTextureDesc::Bilinear)
    {
        // Generate mipmaps only if required
        glGenerateMipmap(GL_TEXTURE_2D);
    }
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
    const std::string& source;
};

Handle<GfxShader> Backend::genShader(const GfxShaderDesc& desc, ErrString* err)
{
    GlShaderDesc oglDescs[] =
    {
        {"vertex", GL_VERTEX_SHADER, desc.src->vert},
        {"fragment", GL_FRAGMENT_SHADER, desc.src->frag},
        {"geometry", GL_GEOMETRY_SHADER, desc.src->geom},
        // FIXME: Also implement `src.tes`, `src.tcs` if have appropriate EXTs
    };

    GLuint oglShaders[5] = {0, 0, 0, 0, 0}; // vert, frag, geom, tes, tcs
    GLuint oglProgram = 0;
    ErrString oglErr;

    for(int i = 0; i < sizeof(oglDescs) / sizeof(GlShaderDesc); i ++)
    {
        const GlShaderDesc& oglDesc = oglDescs[i];
        if(oglDesc.source.empty())
        {
            // Empty shader, go on with the next
            continue;
        }

        oglErr = compileShader(oglShaders[i], oglDesc.type, oglDesc.source.c_str());
        if(oglErr)
        {
            if(err)
            {
                std::ostringstream errMsg;
                errMsg << oglDesc.name << " shader compile error:\n" << oglErr;
                *err = errMsg.str();
            }
            return {};
        }
    }

    oglErr = linkShaderProgram(oglProgram,
                               oglShaders, oglShaders + sizeof(oglShaders) / sizeof(GLuint));

    // Delete the shaders after attaching them to the program because they are not needed anymore
    for(int i = 0; i < sizeof(oglDescs) / sizeof(GlShaderDesc); i ++)
    {
        if(oglShaders[i] != 0)
        {
            glDeleteShader(oglShaders[i]);
        }
    }

    if(oglErr)
    {
        if(err)
        {
            std::ostringstream errMsg;
            errMsg << "Shader program link error:\n" << oglErr;
            *err = errMsg.str();
        }
        return {};
    }

    // Query if the `Uniforms` uniform interface block exists in the shader; if
    // it does, bind its index to 0
    GLuint uniformsIndex = glGetUniformBlockIndex(oglProgram, "Uniforms");
    if(uniformsIndex != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(oglProgram, uniformsIndex, 0);
    }

    // Find any `u_TextureN` uniforms in the shaders and bind them to texture unit
    // `N`, for `N` in [0..MAX_TEXTURES]
    // If GLSL supported `uniform sampler2D u_Texture0 = 1` this would not be required...
    // TODO Find a more elegant way to do texture unit binding!
    std::ostringstream textureUniformName;
    for(unsigned int i = 0; i < GfxCmd::MAX_TEXTURES; i ++)
    {
        textureUniformName.str("");
        textureUniformName << "u_Texture" << i;

        GLint textureUniformIndex = glGetUniformLocation(oglProgram, textureUniformName.str().c_str());
        if(textureUniformIndex >= 0)
        {
            glUniform1i(textureUniformIndex, i); // Bind `u_Texture$i` to `GL_TEXTURE$i`
        }
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

    // TODO Rebuild all pass FBOs here since the textures should have changed in size by the user?
    //      Technically resizing a texture attached to a FBO is perfectly legal but it seems like
    //      it breaks some OpenGL implementations...
}

void Backend::switchToPass(U8 nextPassId)
{
    const GfxPipeline::Pass& pass = pipeline_->passes[nextPassId];
    const PassData& curPassData = passData_[curPassId_];
    const PassData& nextPassData = passData_[nextPassId];

    // Bind the fbo of the next pass if it changed
    if(curPassData.fbo != nextPassData.fbo)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, nextPassData.fbo);
    }

    // Clear the next pass' buffers if required
    if(pass.clearTargets)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // Bind the program of the next pass if it changed
    if(curPassData.program != nextPassData.program)
    {
        glUseProgram(nextPassData.program);
    }

    // Rebind the pass' uniform buffer of the next pass if it changed.
    // See the end of `genShader()`'s implementation
    if(curPassData.ubo != nextPassData.ubo)
    {
        if(nextPassData.ubo != 0)
        {
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, nextPassData.ubo);
        }
        glBindBuffer(GL_UNIFORM_BUFFER, nextPassData.ubo);
    }

    curPassId_ = nextPassId;
}

ErrString Backend::createPassFbo(U8 passId)
{
    GLuint& fbo = passData_[passId].fbo;
    const GfxPipeline::Pass& pass = pipeline_->passes[passId];

    if(pass.targets[0] == GfxPipeline::Pass::SCREEN_TARGET)
    {
        // Render to the default framebuffer (i.e. the framebuffer with id 0)
        fbo = 0;
        return {};
    }

    std::ostringstream err;
    err << "FBO for pass " << passId + 1;

    glGenFramebuffers(1, &fbo);
    if(!fbo)
    {
        err << ": Failed to create FBO";
        return err.str();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GLenum colorAttachments[GfxPipeline::Pass::MAX_TARGETS];
    unsigned int nColorAttachments = 0;
    bool hasDepthAttachment = false;

    // TODO Error out if `pass.nTargets > GfxPipeline::Pass::MAX_TARGETS`?
    for(size_t i = 0; i < pass.nTargets; i ++)
    {
        Handle<GfxTexture> target = pass.targets[i];
        auto descIt = textures_.find(target);
        if(descIt == textures_.end())
        {
            err << ": Nonexisting target texture " << target;
            return err.str();
        }

        if(!descIt->second.desc.format.isDepth())
        {
            GLenum attachment = GL_COLOR_ATTACHMENT0 + nColorAttachments;
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   attachment, GL_TEXTURE_2D,
                                   target, 0);
            colorAttachments[nColorAttachments] = attachment;
            nColorAttachments ++;
        }
        else
        {
            if(hasDepthAttachment)
            {
                err << ": More than one depth target";
                return err.str();
            }
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                   target, 0);
            hasDepthAttachment = true;
        }
    }

    glDrawBuffers(nColorAttachments, colorAttachments);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        err << ": FBO incomplete (status 0x" << std::hex << status << ")";
        return err.str();
    }

    return {};
}


static constexpr const GLenum GFX_VERTEXATTRIB_TYPE_TO_GL[] = // Index by `GfxPipeline::VertexAttrib::Type`
{
    GL_FLOAT, // 0: F32
    GL_INT, // 1: I32
    GL_UNSIGNED_INT, // 2: U32
};
static constexpr const size_t GFX_VERTEXATTRIB_TYPE_SIZE[] = // Index by `GfxPipeline::VertexAttrib::Type`
{
    sizeof(F32), // 0: F32
    sizeof(I32), // 1: I32
    sizeof(U32), // 2: U32
};

Backend::Vao::Vao(const GfxPipeline::Pass& pass, VaoKey key)
    : key(key), vao_(0)
{
    glGenVertexArrays(1, &vao_);
    if(!vao_)
    {
        return;
    }

    glBindVertexArray(vao_);

    // Calculate the stride for all attributes in the vertex buffer and all
    // attributes in the index buffer.
    // Required because the data is interleaved and not tightly packed, so the
    // stride must be passed to `glVertexAttribPointer`
    size_t vertexStride = 0, instanceStride = 0;
    for(unsigned int i = 0; i < pass.nVertexAttribs; i ++)
    {
        const auto& attrib = pass.vertexAttribs[i];
        size_t& attribStride = attrib.instanceDivisor == 0 ? vertexStride : instanceStride;
        attribStride += GFX_VERTEXATTRIB_TYPE_SIZE[unsigned(attrib.type)] * attrib.n;
    }

    // Enable all attribs, binding either the vertex or instance buffer and
    // incrementing attribute offsets as needed
    GLuint boundBuffer = -1;
    size_t vertexOffset = 0, instanceOffset = 0;
    for(unsigned int i = 0; i < pass.nVertexAttribs; i ++)
    {
        const auto& attrib = pass.vertexAttribs[i];

        const GLuint* attribBuffer;
        const size_t* attribStride;
        size_t* attribOffset;
        if(attrib.instanceDivisor == 0)
        {
            attribBuffer = &key.vertexBuffer;
            attribStride = &vertexStride;
            attribOffset = &vertexOffset;
        }
        else
        {
            attribBuffer = &key.instanceBuffer;
            attribStride = &instanceStride;
            attribOffset = &instanceOffset;
        }

        if(boundBuffer != *attribBuffer)
        {
            glBindBuffer(GL_ARRAY_BUFFER, *attribBuffer);
            boundBuffer = *attribBuffer;
        }
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i,
                              attrib.n, GFX_VERTEXATTRIB_TYPE_TO_GL[unsigned(attrib.type)],
                              GL_FALSE,
                              *attribStride, (void*)*attribOffset);

        *attribOffset += GFX_VERTEXATTRIB_TYPE_SIZE[unsigned(attrib.type)] * attrib.n;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, key.indexBuffer);
}

Backend::Vao::~Vao()
{
    if(vao_)
    {
        glDeleteVertexArrays(1, &vao_); vao_ = 0;
    }
}


// === CmdFuncs assume that some state has been bound for them by `runCmds()` ===
// - shader program
// - FBO
// - VAO
// - All textures
// - UBO

// TODO Add a "rendering mode" enum to the GfxPipeline::Pass to switch primitive
//      type instead of just using GL_TRIANGLES?

using CmdFunc = void(*)(const GfxCmd& cmd);

inline static void cmdDraw(const GfxCmd& cmd)
{
    glDrawArrays(GL_TRIANGLES, cmd.first, cmd.n);
}

inline static void cmdDrawIndexed(const GfxCmd& cmd)
{
    glDrawElements(GL_TRIANGLES, cmd.n, GL_UNSIGNED_INT,
                   (void*)(cmd.first * sizeof(GLuint)));
}

inline static void cmdDrawInstanced(const GfxCmd& cmd)
{
    glDrawArraysInstanced(GL_TRIANGLES, cmd.first, cmd.n,
                          cmd.nInstances);
}

inline static void cmdDrawInstancedIndexed(const GfxCmd& cmd)
{
    glDrawElementsInstanced(GL_TRIANGLES, cmd.n, GL_UNSIGNED_INT,
                            (void*)(cmd.first * sizeof(GLuint)),
                            cmd.nInstances);
}


static constexpr const CmdFunc GFX_CMD_FUNCS[] = // Indexed by `GfxCmd::Op`
{
    cmdDraw, // 0: Draw
    cmdDrawIndexed, // 1: DrawIndexed
    cmdDrawInstanced, // 2: DrawInstanced
    cmdDrawInstancedIndexed, // 3: DrawInstancedIndexed
};

void Backend::runCmds(const GfxCmd* cmds, const GfxCmdIndex* cmdsOrder, size_t n)
{
    for(size_t i = 0; i < n; i ++)
    {
        const GfxCmd& cmd = cmds[cmdsOrder[i].index];

        // Switch to the right pass if needed; rebinds the FBO and shader program
        // The sorting key specifies all cmds for a specific pass to be run in sequence
        if(cmd.passId != curPassId_)
        {
            switchToPass(cmd.passId);
        }

        // Bind the right textures if needed
        for(unsigned int i = 0; i < cmd.nTextures; i ++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, cmd.textures[i]);
        }

        // Bind the right vertex/instance attribute source buffers (via rebinding a VAO) if needed
        VaoKey cmdVaoKey = {cmd.passId, cmd.vertexBuffer, cmd.indexBuffer, cmd.instanceBuffer};
        if(curBindings_.vaoKey != cmdVaoKey)
        {
            // [Create] + switch to the required VAO
            auto vaoIt = vaos_.find(cmdVaoKey);
            if(vaoIt == vaos_.end())
            {
                const GfxPipeline::Pass& pass = pipeline_->passes[curPassId_];
                vaoIt = vaos_.insert(std::make_pair(cmdVaoKey, Vao(pass, cmdVaoKey))).first;
            }

            glBindVertexArray(vaoIt->second);
            curBindings_.vaoKey = cmdVaoKey;
        }

        // Issue the actual command
        auto func = GFX_CMD_FUNCS[unsigned(cmd.op)];
        func(cmd);
    }
}

}
}
