#pragma once

#include <unordered_map>
#include <unordered_set>
#include <flextGL.h>
#include "../GfxBackend.hh"

namespace Ares
{
namespace GL33
{

class Backend : public Ares::GfxBackend
{
private:
    struct TextureDesc
    {
        GfxTextureDesc desc;
        GLenum format, internalFormat;
    };
    std::unordered_map<Handle<GfxBuffer>, GfxBufferDesc> buffers_;
    std::unordered_map<Handle<GfxTexture>, TextureDesc> textures_;
    std::unordered_set<Handle<GfxShader>> shaders_;

public:
    Backend();
    ErrString init() override;
    ~Backend() override;

    Handle<GfxBuffer> genBuffer(const GfxBufferDesc& desc, ErrString* err) override;
    void resizeBuffer(Handle<GfxBuffer> buffer, size_t newSize) override;
    void editBuffer(Handle<GfxBuffer> buffer, size_t dataOffset, size_t dataSize, const void* data) override;
    void delBuffer(Handle<GfxBuffer> buffer) override;

    Handle<GfxTexture> genTexture(const GfxTextureDesc& desc, ErrString* err) override;
    void resizeTexture(Handle<GfxTexture> texture, Resolution newResolution) override;
    void editTexture(Handle<GfxTexture> texture, ViewRect dataRect, const void* data) override;
    void delTexture(Handle<GfxTexture> texture) override;

    Handle<GfxShader> genShader(const GfxShaderDesc& desc, ErrString* err) override;
    void delShader(Handle<GfxShader> shader) override;

    void changeResolution(Resolution resolution) override;
    void runCmds(const GfxCmd* cmds, const GfxCmdIndex* cmdsOrder, size_t n) override;
};

}
}
