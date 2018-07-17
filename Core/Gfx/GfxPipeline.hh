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
        static constexpr const unsigned int MAX_VERTEX_ATTRIBS = 4;
        static constexpr const unsigned int MAX_TARGETS = 8;
        static constexpr const unsigned int MAX_TEXTURES = 8;

        static constexpr const Handle<GfxTexture> SCREEN_TARGET{U32(-2)};
        static constexpr const Handle<GfxTexture> DEPTH_TARGET{U32(-3)};

        VertexAttrib vertexAttribs[MAX_VERTEX_ATTRIBS] = {};
        unsigned int nVertexAttribs = 0;

        Handle<GfxTexture> target[MAX_TARGETS] = {SCREEN_TARGET};
        unsigned int nTargets = 1;

        Handle<GfxShader> shader{0};
    };

    std::vector<Pass> passes; // Do not exceed `MAX_PASSES` passes
};

}
