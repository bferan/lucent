#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265358979323846

layout(location = 0) attribute vec3 a_Position;
layout(location = 1) attribute vec3 a_Normal;
layout(location = 2) attribute vec3 a_Tangent;
layout(location = 3) attribute vec3 a_Bitangent;
layout(location = 4) attribute vec2 a_UV;

layout(location = 0) varying vec3 v_Direction;

layout(location = 0) out vec4 o_Color;

layout(set = 0, binding = 0) uniform Global
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Roughness;
};
layout(set = 0, binding = 1) uniform sampler2D u_RectTex;

void vert()
{
    v_Direction = a_Position;
    vec4 pos = u_Proj * vec4(mat3(u_View) * a_Position, 1.0);
    gl_Position = pos.xyww;
}

void frag()
{
    vec3 dir = normalize(v_Direction);
    float lon = atan(dir.x, -dir.z)/(2*PI) + .5;
    float lat = asin(dir.y)/PI + .5;

    vec3 col = texture(u_RectTex, vec2(lon, lat)).rgb;
    o_Color = vec4(col, 1.0);
}

