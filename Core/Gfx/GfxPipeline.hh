#pragma once

#include <vector>
#include "../Base/NumTypes.hh"
#include "../Base/Handle.hh"
#include "GfxResources.hh"
#include "GfxCmd.hh"

namespace Ares
{

/// A graphics pipeline.
/// A pipeline can contain [0..MAX_PASSES] `Pass`es whose commands are run one
/// after the other (first the commands for pass 0, then for pass 1, ...) in a pipeline
/// fashion.
struct GfxPipeline
{
    /// The maximum number of `passes` that can be in a pipeline.
    static constexpr const unsigned int MAX_PASSES = (1 << (sizeof(GfxCmd::passId) * 8)) - 1;


    /// The description of a input vertex or instance attribute.
    /// Attribute values are transferred once per vertex from the vertex buffer (if
    /// `instanceDivisor == 0`) or once per instance form the instance buffer (if
    /// `instanceDivisor > 0`) to shader inputs.
    /// Based on OpenGL's "vertex attributes" and "instance attributes".
    struct Attrib
    {
        /// A human-readable, unique for the pass name for the attribute.
        const char* name = nullptr;
        enum class Type : U8
        {
            F32 = 0,
            I32 = 1,
            U32 = 2,
        } type = Type::F32; ///< The type of the attribute's data.

        /// The number of `type` items in the attribute's data.
        unsigned int n = 4;

        /// The instance divisor for the attribute. If `instanceDivisor > 0`, the
        /// attribute will be read once per `instanceDivisor` instances instead
        /// of once per vertex; see `Attrib`'s documentation.
        unsigned int instanceDivisor = 0;
    };

    /// A single pass of the pipeline.
    /// A pass describes:
    /// - The output targets (textures) that the pass' commands will render to
    ///   (and wether to clear them when the pass is started or not)
    /// - The input vertex + instance attributes the pass' shader will get
    /// - The shader to use for the pass' commands and its (optional) uniform buffer
    struct Pass
    {
        /// The maximum number of `Attrib`s `attribs` can hold.
        static constexpr const unsigned int MAX_ATTRIBS = 8;

        /// The maximum number of `GfxTexture` targets `targets` can hold.
        static constexpr const unsigned int MAX_TARGETS = 8;

        /// A special target texture that, when set as the only target in `targets`,
        /// will make the renderer output directly to the screen instead of to
        /// textures.
        static const Handle<GfxTexture> SCREEN_TARGET;


        /// The vertex + instance attributes for this pass. See `Pass`' documentation.
        /// It is perfectly legal not to specify any vertex attribute for the pass.
        Attrib attribs[MAX_ATTRIBS] = {};

        /// The number of attributes in `attribs`. Must be `<= MAX_ATTRIBS`.
        unsigned int nAttribs = 0;


        /// The target textures this pass will output to. See `Pass`' documentation
        /// and `SCREEN_TARGET`.
        /// The textures should have been `genTexture()`d from the `GfxBackend`
        /// in use **before** the pipeline is set.
        ///
        /// For performance, all target extures should use nearest or bilinear
        /// filtering and **no mipmapping**.
        /// Target textures are **not** resized automatically if the viewport
        /// changes, so they should each be resized manually when needed.
        Handle<GfxTexture> targets[MAX_TARGETS] = {SCREEN_TARGET};

        /// The number of target textures in `targets`. Must be `<= MAX_TARGETS`.
        unsigned int nTargets = 1;

        /// `true` if the `target` textures/screen should be cleared when the pass
        /// is started.
        bool clearTargets = true;


        /// The shader program that the pass will use for transforming/shading geometry.
        /// The shader should have been `genShader()`d from the `GfxBackend`
        /// in use **before** the pipeline is set.
        Handle<GfxShader> shader{0};

        /// The [optional] buffer containing uniform data for `shader`.
        /// The buffer should have been `genBuffer()`d from the `GfxBackend`
        /// in use **before** the pipeline is set; its data can be updated at any
        /// time by the user afterwards.
        Handle<GfxBuffer> uniformBuffer{0};
    };


    /// The list of passes in the pipeline. Do not exceed `MAX_PASSES` passes!
    std::vector<Pass> passes;
};

}
