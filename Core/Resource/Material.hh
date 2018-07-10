#pragma once

#include "../Base/Ref.hh"
#include "../Resource/Image.hh"
#include "../Visual/Color.hh"

namespace Ares
{

/// A gfx material.
/// Based on GLTF 2.0's PBR metallic-roughness materials.
struct Material
{
    /// A linear albedo (base color) for the material, including alpha.
    RGBAF albedoFac{1.0f, 1.0f, 1.0f, 1.0f};

    /// The texture defining the albedo (base color) of the material, including
    /// alpha.
    /// **This texture is sRGB and must be converted to linear before shading.**
    /// If null, only `albedoFac` will be used.
    /// If not null, the linearized texture will be multiplied by `albedoFac`
    /// per-component to get the final color value.
    Ref<Image<RGBA8>> albedoTex;

    enum AlphaMode
    {
        Opaque, ///< Material is fully opaque; alpha is ignored.
        Mask, ///< Material is opaque where alpha > `alphaCutoff`, transparent where not.
        Blend, ///< Material is alpha-blended with the background.

    } alphaMode = Mask; ///< The mode of interpreting `albedo{Fac,Tex}`'s alpha value.

    /// The alpha cutoff value. See `alphaMode`.
    float alphaCutoff{1.0f};

    /// The base metallicity of the material.
    float metallicFac{1.0f};

    /// The base roughness of the material.
    float roughnessFac{1.0f};

    /// The texture defining the ambient occlusion, roughness and metallicity of
    /// the material.
    /// Red channel is AO, blue channel is roughness, green channel is metallicity.
    /// If null, only `metallicFac` and `roughnessFac` will be used, with no AO.
    /// If not null, corresponding channels in the texture will be multiplied by
    /// `metallicFac` and `roughnessFac` to get the final metallicity and roughness
    /// value.
    Ref<Image<RGB8>> ormTex;

    /// The texture defining the normal map of the material in tangent space.
    /// If null, default normal is calculated for flat shading.
    Ref<Image<RGB8>> normalTex;

    /// The texture defining the emitted light color of the material.
    /// **This texture is sRGB and must be converted to linear before shading.**
    /// If null, default emission is a constant (0, 0, 0).
    Ref<Image<RGB8>> emissionTex;
};

}
