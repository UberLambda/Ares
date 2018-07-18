#version 330 core

in vec2 vo_UV;

uniform sampler2D u_ColorBuffer; // RGBA16F (linear); RGB = color buffer, A = alpha

layout(location = 0) out vec4 fo_Color; // RGBA8 (sRGB); RGB = pixel color, A = pixel alpha

void main()
{
    // Simplified Reinhard tonemapping (without taking luminance into account)
    // The color will be converted from linear to sRGB by OpenGL automagically
    vec4 hdrColor = texture(u_ColorBuffer, vo_UV);
    fo_Color = vec4(hdrColor.rgb / (hdrColor.rgb + 1.0),
                    hdrColor.a);
}
