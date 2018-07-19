#version 330 core

in vec2 vo_UV;

uniform sampler2D u_Texture0; // Color; RGBA16F (linear); RGB = color, A = alpha
uniform sampler2D u_Texture1; // Normal; RGB10_A2 (linear); RGB = normal
uniform sampler2D u_Texture2; // RMID; RGBA8 (linear); R = roughness, G = metallicity,
                              // B+A = U16 (big endian) entity id
uniform sampler2D u_Texture3; // Velocity; RGB16F (linear); RGB = velocity
uniform sampler2D u_Texture4; // Depth32F

out vec4 fo_Color; // RGBA (sRGB)

void main()
{
    // Simplified Reinhard tonemapping (without taking luminance into account)
    // The color will be converted from linear to sRGB by OpenGL automagically
    vec4 hdrColor = texture(u_Texture0, vo_UV);
    fo_Color = vec4(hdrColor.rgb / (hdrColor.rgb + 1.0),
                    hdrColor.a);
}
