#pragma once

#include "../Base/Handle.hh"
#include "../Base/NumTypes.hh"
#include "GfxResources.hh"

namespace Ares
{

/// A single rendering command to be run by a `GfxRenderer`.
///
/// Attribute (`GfxPipeline::Attrib`) data for different attributes in `vertexBuffer`
/// and `instanceBuffer` is interleaved and not tightly packed; see OpenGL's definitons for this.
struct GfxCmd
{
    /// The maximum number of textures that can be bound at a time for the command.
    static const unsigned int MAX_TEXTURES = 4;


    enum Op : U8
    {
        /// Draws `n` vertices (whose `GfxPipeline::Attrib`s are gathered from
        /// `vertexBuffer`) starting from the vertex `first`
        /// in the vertex buffer.
        Draw = 0,

        /// Draws `n` indexed vertices (whose `GfxPipeline::Attrib`s are gathered from
        /// `vertexBuffer`) whose U32 indices are in `indexBuffer`, starting from
        /// the index `first` in the index buffer.
        DrawIndexed = 1,

        /// Performs a `Draw` operation `nInstance` times, gathering per-instance
        /// `GfxPipeline::Attrib` data from `instanceBuffer`.
        DrawInstanced = 2,

        /// Performs a `DrawIndexed` operation `nInstance` times, gathering per-instance
        /// `GfxPipeline::Attrib` data from `instanceBuffer`.
        DrawIndexedInstanced = 3,

    } op = Draw; ///< The rendering operation to perform for this command.


    /// The id of the `Pass` this command belongs to in the `GfxPipeline`.
    ///
    /// The commands of pass `0` are run first, followed by the ones of pass `1`,
    /// etc. in sequence.
    U8 passId = 0;

    /// The buffer where to source per-vertex attribs from. See `Op::Draw`.
    /// The buffer should have been `genBuffer()`d from the `GfxBackend` in use
    /// or the handle be left null to use no such buffer.
    Handle<GfxBuffer> vertexBuffer;

    /// The buffer where to source U32 indices from. See `Op::DrawIndexed`.
    /// The buffer should have been `genBuffer()`d from the `GfxBackend` in use
    /// or the handle be left null to use no such buffer.
    Handle<GfxBuffer> indexBuffer;

    /// The buffer where to source per-instance attribs from. See `Op::DrawInstanced`.
    /// The buffer should have been `genBuffer()`d from the `GfxBackend` in use
    /// or the handle be left null to use no such buffer.
    Handle<GfxBuffer> instanceBuffer;

    /// The first vertex or index to draw; see `Op::Draw`, `Op::DrawIndexed`.
    unsigned int first = 0;

    /// The number of vertex or indices to draw; see `Op::Draw`, `Op::DrawIndexed`.
    unsigned int n = 0;

    /// The number of instances to draw; see `Op::DrawInstanced`
    unsigned int nInstances = 0;

    /// The textures to be bound to texture inputs in the `GfxPipeline::Pass`'
    /// shader for this command.
    ///
    /// The renderer will try to minimize the amount of times textures are bound
    /// by clustering drawcalls that use the same textures togeter.
    Handle<GfxTexture> textures[MAX_TEXTURES];

    /// The number of textures in `nTextures`. Must be `<= MAX_TEXTURES`.
    unsigned int nTextures = 0;
};


/// The index of a `GfxCmd` in a sorted list.
///
/// Commands are sorted by the value of the `key`; for `N` commands in a `cmdList`,
/// `cmdList[idx0.index]` for the `idx0` with the lowest `key` is run, followed
/// by `cmdList[idx1.index]` for the second lowest `key` in `idx1`, etc. etc.
struct GfxCmdIndex
{
    U64 key; ///< The command's sort key in the list. See `GfxCmdIndex`'s documentation.
    size_t index; ///< The index of the command in the list. See `GfxCmdIndex`'s documentation.

    /// `true` if this' `key` has a lower value than other's.
    inline bool operator<(const GfxCmdIndex& other) const
    {
        return key < other.key;
    }
};

}
