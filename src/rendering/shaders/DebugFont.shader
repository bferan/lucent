#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) attribute vec3 a_Position;
layout(location = 1) attribute vec3 a_Normal;
layout(location = 2) attribute vec3 a_Tangent;
layout(location = 3) attribute vec3 a_Bitangent;
layout(location = 4) attribute vec2 a_UV;
layout(location = 5) attribute vec4 a_Color;

layout(location = 0) varying vec2 v_UV;
layout(location = 1) varying vec4 v_Color;

layout(location = 0) out vec4 o_Color;

layout(set = 0, binding = 0) uniform sampler2D u_FontTex;

void vert()
{
    v_UV = a_UV;
    v_Color = a_Color;
    gl_Position = vec4(a_Position, 1.0);
}

void frag()
{
    float value = texture(u_FontTex, v_UV).r;
    o_Color = value * v_Color;
}


