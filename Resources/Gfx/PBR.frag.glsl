#version 330 core

// TODO: Actually all PBR shading...

in vec4 vo_Color0;

layout(location = 0) out vec4 fo_Color; // RGBA16F (linear); RGB = color, A = alpha
layout(location = 1) out vec4 fo_Normal; // RGB10_A2 (linear); RGB = normal
layout(location = 2) out vec4 fo_RMID; // RGBA8 (linear); R = roughness, G = metallicity,
                                       // I+D = U16 (big endian) entity id
layout(location = 4) out vec2 fo_Velocity; // RGB16F; RGB = velocity

void main()
{
    fo_Color = vo_Color0;
}
