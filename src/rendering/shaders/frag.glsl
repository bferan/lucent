#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 iUV;
layout(location = 1) in vec3 iTangent;
layout(location = 2) in vec3 iBitangent;
layout(location = 3) in vec3 iNormal;
layout(location = 4) in vec3 iPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform samplerCube sEnvIrradiance;
layout(set = 0, binding = 2) uniform samplerCube sEnvSpecular;
layout(set = 0, binding = 3) uniform sampler2D sBRDF;

layout(set = 0, binding = 0) uniform MyBuffer
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos;
};

layout(set = 1, binding = 0) uniform sampler2D sBaseColor;
layout(set = 1, binding = 1) uniform sampler2D sMetalRoughness;
layout(set = 1, binding = 2) uniform sampler2D sNormal;
layout(set = 1, binding = 3) uniform sampler2D sAO;
layout(set = 1, binding = 4) uniform sampler2D sEmissive;

const vec3 kDielectricSpecular = vec3(0.04);
const vec3 kBlack = vec3(0.0);

void main()
{
    vec4 metallicRoughness = texture(sMetalRoughness, iUV);
    vec3 base = texture(sBaseColor, iUV).rgb;
    base *= vec3(0.588);

    vec3 t = normalize(iTangent);
    vec3 b = normalize(iBitangent);
    vec3 n = normalize(iNormal);

    vec3 nTex = texture(sNormal, iUV).rgb * 2.0 - vec3(1.0);
    vec3 N = mat3(t, b, n) * normalize(nTex);

    float metal = metallicRoughness.b;
    float rough = metallicRoughness.g;

    vec3 shaded = vec3(0.0);

    vec3 albedo = mix(base * (1 - kDielectricSpecular.r), kBlack, metal);
    vec3 F0 = mix(kDielectricSpecular, base, metal);
    float a2 = rough * rough * rough * rough;

    vec3 V = normalize(uCameraPos - iPos);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    vec3 fsRough = F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(1.0 - max(dot(N, V), 0.0), 5.0);
    vec3 kD = 1.0 - fsRough;

    vec3 R = 2.0 * dot(V, N)*N - V;
    vec3 F = F0 + (1.0 - F0) * pow((1.0 - max(dot(N, V), 0.0)), 5.0);

    R.y = -R.y; // TMP
    vec3 envSpecular = textureLod(sEnvSpecular, R, rough * 5.0).rgb;

    vec2 brdf = texture(sBRDF, vec2(NdotV, rough)).rg;

    N.y = -N.y; // TMP
    vec3 ambient = kD * albedo * texture(sEnvIrradiance, N).rgb;
    ambient += envSpecular * (F * brdf.x + brdf.y);

    float ao = texture(sAO, iUV).r;

    shaded += ao * ambient;
    shaded += texture(sEmissive, iUV).rgb;

    outColor = vec4(shaded, 1.0);
}