#pragma once

#include <vector>
#include "../Base/NumTypes.hh"
#include "../Base/Handle.hh"
#include "GfxResources.hh"
#include "GfxCmd.hh"

namespace Ares
{

struct GfxPipeline
{
    static constexpr const unsigned int MAX_PASSES = (1 << (sizeof(GfxCmd::passId) * 8)) - 1;

    struct VertexAttrib
    {
        const char* name = nullptr;
        enum class Type : U8
        {
            F32 = 0,
            I32 = 1,
            U32 = 2,
        } type = Type::F32;
        unsigned int n = 4;
        unsigned int instanceDivisor = 0;
    };

    struct Pass
    {
        static constexpr const unsigned int MAX_VERTEX_ATTRIBS = 8;
        static constexpr const unsigned int MAX_TARGETS = 8;
        static constexpr const unsigned int MAX_TEXTURES = 4;

        static const Handle<GfxTexture> SCREEN_TARGET; // Special texture handle to the screen's render target

        VertexAttrib vertexAttribs[MAX_VERTEX_ATTRIBS] = {};
        unsigned int nVertexAttribs = 0;

        // For performance, all textures in `targets` should use Nearest or Bilinear
        // filtering and **no mipmapping**!
        Handle<GfxTexture> targets[MAX_TARGETS] = {SCREEN_TARGET};
        unsigned int nTargets = 1;
        bool clearTargets = true;

        Handle<GfxShader> shader{0};
    };

    std::vector<Pass> passes; // Do not exceed `MAX_PASSES` passes
};

}
