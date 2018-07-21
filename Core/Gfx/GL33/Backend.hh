#pragma once

#include <unordered_map>
#include <unordered_set>
#include <flextGL.h>
#include "../GfxBackend.hh"
#include "../Base/KeyString.hh"

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

    Ref<GfxPipeline> pipeline_;
    U8 curPassId_ = 0;
    struct PassData
    {
        GLuint fbo = -1;
        GLuint program = -1;
        GLuint ubo = -1;
    };
    std::vector<PassData> passData_;


    struct VaoKey
    {
        U8 passId;
        GLuint vertexBuffer;
        GLuint indexBuffer;
        GLuint instanceBuffer;

        inline bool operator==(const VaoKey& other) const
        {
            return passId == other.passId
                   && vertexBuffer == other.vertexBuffer
                   && indexBuffer == other.indexBuffer
                   && instanceBuffer == other.instanceBuffer;
        }
        inline bool operator!=(const VaoKey& other) const
        {
            return !operator==(other);
        }
    };

    struct Vao
    {
        VaoKey key;

    private:
        GLuint vao_;

    public:
        Vao(const GfxPipeline::Pass& pass, VaoKey key);
        ~Vao();

        Vao(const Vao& toCopy) = delete;
        Vao(Vao&& toMove)
        {
            // Move data over
            key = toMove.key;
            vao_ = toMove.vao_;

            // Invalidate the moved instance
            toMove.vao_ = 0;
        }

        inline operator GLuint() const
        {
            return vao_;
        }
    };
    class VaoKeyHasher
    {
    public:
        inline size_t operator()(const VaoKey& value) const
        {
            // Poor man's solution for hashing 3 U32 keys and 1 U8 key together
            static constexpr const size_t KEY_SIZE = sizeof(GLuint) * 3 + sizeof(U8) + 1;

            char keyBytes[KEY_SIZE];
            GLuint* keyGLuints = (U32*)keyBytes;
            keyGLuints[0] = value.vertexBuffer;
            keyGLuints[1] = value.indexBuffer;
            keyGLuints[2] = value.instanceBuffer;
            keyBytes[KEY_SIZE - 2] = value.passId;
            keyBytes[KEY_SIZE - 1] = '\0';

            KeyString<KEY_SIZE> key(keyBytes);
            return key.hash();
        }
    };

    using VaoMap = std::unordered_map<VaoKey, Vao, VaoKeyHasher>;
    VaoMap vaos_;

    struct Bindings
    {
        VaoKey vaoKey = {0, U32(-1), U32(-1), U32(-1)}; // Encapsulates vertex + index + instance buffers
        GLuint ubo = 0;

    } curBindings_;


    ErrString createPassFbo(U8 passId);
    void switchToPass(U8 nextPassId);

public:
    Backend();
    ErrString init(Ref<GfxPipeline> pipeline) override;
    ~Backend() override;

    Handle<GfxBuffer> genBuffer(const GfxBufferDesc& desc) override;
    void resizeBuffer(Handle<GfxBuffer> buffer, size_t newSize) override;
    void editBuffer(Handle<GfxBuffer> buffer, size_t dataOffset, size_t dataSize, const void* data) override;
    void delBuffer(Handle<GfxBuffer> buffer) override;

    Handle<GfxTexture> genTexture(const GfxTextureDesc& desc) override;
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
