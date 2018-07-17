#pragma once

#include "../Base/Handle.hh"
#include "../Base/NumTypes.hh"
#include "GfxResources.hh"

namespace Ares
{

struct GfxCmd
{
    static const unsigned int MAX_TEXTURES = 4;

    enum Op : U8
    {
        Draw = 0,
        DrawIndexed = 1,
        DrawInstanced = 2,
        DrawIndexedInstanced = 3,

    } op;

    U8 passId;

    Handle<GfxBuffer> vertexBuffer;
    Handle<GfxBuffer> indexBuffer;
    Handle<GfxBuffer> instanceBuffer; // Where to pick data for instanced rendering from
    unsigned int first; // First vertex for Draw[Instanced], first index for DrawIndexed[Instanced]
    unsigned int n; // N of vertices for Draw[Instanced], N of indices for DrawIndexed[Instanced]
    unsigned int nInstances; // N of instances for Draw[Indexed]Instanced

    Handle<GfxTexture> textures[MAX_TEXTURES];
    Handle<GfxBuffer> uniformsBuffer;
};

struct GfxCmdIndex
{
    U64 key;
    size_t index;

    inline bool operator<(const GfxCmdIndex& other) const
    {
        return key < other.key;
    }
};

}
