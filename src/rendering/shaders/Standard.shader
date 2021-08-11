#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "Test.shader"

layout(location = 0) attribute vec3 a_Position;
layout(location = 1) attribute vec3 a_Normal;
layout(location = 2) attribute vec3 a_Tangent;
layout(location = 3) attribute vec3 a_Bitangent;
layout(location = 4) attribute vec2 a_UV;

layout(location = 0) varying vec2 v_UV;
layout(location = 1) varying vec3 v_Tangent;
layout(location = 2) varying vec3 v_Bitangent;
layout(location = 3) varying vec3 v_Normal;
layout(location = 4) varying vec3 v_Pos;

layout(location = 0) out vec4 o_Color;

layout(set = 0, binding = 0) uniform Globals
{
    mat4 u_Model;
    mat4 u_View;
    mat4 u_Proj;
    vec3 u_CameraPos;
};

layout(set = 0, binding = 1) uniform samplerCube u_EnvIrradiance;
layout(set = 0, binding = 2) uniform samplerCube u_EnvSpecular;
layout(set = 0, binding = 3) uniform sampler2D u_BRDF;

layout(set = 1, binding = 0) uniform sampler2D u_BaseColor;
layout(set = 1, binding = 1) uniform sampler2D u_MetalRoughness;
layout(set = 1, binding = 2) uniform sampler2D u_Normal;
layout(set = 1, binding = 3) uniform sampler2D u_AO;
layout(set = 1, binding = 4) uniform sampler2D u_Emissive;

const vec3 kDielectricSpecular = vec3(0.04);
const vec3 kBlack = vec3(0.0);

void vert()
{
    vec4 world = u_Model * vec4(a_Position, 1.0);

    v_UV = a_UV;
    v_Tangent   = normalize(mat3(u_Model) * a_Tangent);
    v_Bitangent = normalize(mat3(u_Model) * a_Bitangent);
    v_Normal    = normalize(mat3(u_Model) * a_Normal);
    v_Pos = world.xyz;

    gl_Position = u_Proj * u_View * world;
}

void frag()
{
    vec4 metallicRoughness = texture(u_MetalRoughness, v_UV);
    vec3 base = texture(u_BaseColor, v_UV).rgb;
    base *= vec3(0.588);

    vec3 t = normalize(v_Tangent);
    vec3 b = normalize(v_Bitangent);
    vec3 n = normalize(v_Normal);

    vec3 nTex = texture(u_Normal, v_UV).rgb * 2.0 - vec3(1.0);
    vec3 N = mat3(t, b, n) * normalize(nTex);

    float metal = metallicRoughness.b;
    float rough = metallicRoughness.g;

    vec3 shaded = vec3(0.0);

    vec3 albedo = mix(base * (1 - kDielectricSpecular.r), kBlack, metal);
    vec3 F0 = mix(kDielectricSpecular, base, metal);
    float a2 = rough * rough * rough * rough;

    vec3 V = normalize(u_CameraPos - v_Pos);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    vec3 fsRough = F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(1.0 - max(dot(N, V), 0.0), 5.0);
    vec3 kD = 1.0 - fsRough;

    vec3 R = 2.0 * dot(V, N)*N - V;
    vec3 F = F0 + (1.0 - F0) * pow((1.0 - max(dot(N, V), 0.0)), 5.0);

    R.y = -R.y; // TMP
    vec3 envSpecular = textureLod(u_EnvSpecular, R, rough * 5.0).rgb;

    vec2 brdf = texture(u_BRDF, vec2(NdotV, rough)).rg;

    N.y = -N.y; // TMP
    vec3 ambient = kD * albedo * texture(u_EnvIrradiance, N).rgb;
    ambient += envSpecular * (F * brdf.x + brdf.y);

    float ao = texture(u_AO, v_UV).r;

    shaded += ao * ambient;
    shaded += texture(u_Emissive, v_UV).rgb;

    o_Color = vec4(shaded, 1.0);
}