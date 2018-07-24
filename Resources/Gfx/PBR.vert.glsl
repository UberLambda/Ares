#version 330 core

// TODO: All actual PBR shading...

layout(location = 0) in vec3 vi_Position;
layout(location = 1) in vec3 vi_Normal;
layout(location = 2) in vec4 vi_Tangent;
layout(location = 3) in vec2 vi_TexCoord0;
layout(location = 4) in vec2 vi_TexCoord1;
layout(location = 5) in vec4 vi_Color0;
layout(location = 6) in mat4 vi_i_Model; // (uploaded as 4 per-instance vec4s)

uniform Uniforms
{
    mat4 u_ViewProjection;
};

out vec3 vo_Normal;
out vec4 vo_Color0;

void main()
{
    gl_Position = u_ViewProjection * vi_i_Model * vec4(vi_Position, 1.0);
    vo_Normal = vi_Normal;
    vo_Color0 = vi_Color0;
}
