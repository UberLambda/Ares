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

    } op = Draw;

    U8 passId = 0;

    Handle<GfxBuffer> vertexBuffer;
    Handle<GfxBuffer> indexBuffer; // Contains U32 indices
    Handle<GfxBuffer> instanceBuffer; // Where to pick data for instanced rendering from
    unsigned int first = 0; // First vertex for Draw[Instanced], first index for DrawIndexed[Instanced]
    unsigned int n = 0; // N of vertices for Draw[Instanced], N of indices for DrawIndexed[Instanced]
    unsigned int nInstances = 0; // N of instances for Draw[Indexed]Instanced

    Handle<GfxTexture> textures[MAX_TEXTURES];
    unsigned int nTextures = 0;
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
