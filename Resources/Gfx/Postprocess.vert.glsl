#version 330 core

out vec2 vo_UV;

void main()
{
    vo_UV = vec2(float((gl_VertexID & 1) << 2),
                 float((gl_VertexID & 2) << 1)) * 0.5;
    gl_Position = vec4(vo_UV * 2.0 - 1.0, 0.0, 1.0);
}
