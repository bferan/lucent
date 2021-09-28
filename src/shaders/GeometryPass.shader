#include "VertexInput"

layout(location=0) varying Vertex
{
    vec2 v_UV;
    vec3 v_Tangent;
    vec3 v_Bitangent;
    vec3 v_Normal;
};

// GBuffer targets
layout(location=0) out vec4 o_BaseColor;
layout(location=1) out vec3 o_Normal;
layout(location=2) out vec2 o_MetalRoughness;

// Per-draw uniforms
layout(set=0, binding=0) uniform Globals
{
    mat4 u_MVP;
    mat4 u_MV;
};

// Material properties
layout(set=1, binding=0) uniform Material
{
    vec4 u_BaseColorFactor;
    float u_MetallicFactor;
    float u_RoughnessFactor;
};
layout(set=1, binding=1) uniform sampler2D u_BaseColor;
layout(set=1, binding=2) uniform sampler2D u_MetalRoughness;
layout(set=1, binding=3) uniform sampler2D u_Normal;

void Vertex()
{
    vec3 bitangent = a_Tangent.w * cross(a_Normal, vec3(a_Tangent));

    v_UV = a_UV;
    v_Tangent   = normalize(mat3(u_MV) * vec3(a_Tangent));
    v_Bitangent = normalize(mat3(u_MV) * bitangent);
    v_Normal    = normalize(mat3(u_MV) * a_Normal);

    gl_Position = u_MVP * vec4(a_Position, 1.0);
}

void Fragment()
{
    vec4 metallicRoughness = texture(u_MetalRoughness, v_UV);

    vec3 t = normalize(v_Tangent);
    vec3 b = normalize(v_Bitangent);
    vec3 n = normalize(v_Normal);

    vec3 nTex = texture(u_Normal, v_UV).rgb * 2.0 - vec3(1.0);
    nTex.y = -nTex.y;
    vec3 N = mat3(t, b, n) * normalize(nTex);

    vec4 baseColor = texture(u_BaseColor, v_UV) * u_BaseColorFactor;
    float metal = metallicRoughness.b * u_MetallicFactor;
    float rough = metallicRoughness.g * u_RoughnessFactor;

    //rough = max(rough, 0.3);

    o_BaseColor = baseColor;
    o_Normal = 0.5 * N + 0.5;
    o_MetalRoughness = vec2(metal, rough);
}