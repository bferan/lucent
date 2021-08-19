#include "shared/VertexLayout"
#include "shared/PBR"

const float C = 80.0;

layout(location=0) varying Vertex
{
    vec2 v_UV;
    vec3 v_Tangent;
    vec3 v_Bitangent;
    vec3 v_Normal;
    vec3 v_Pos;
    vec4 v_LightPos;
};

layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform Globals
{
    mat4 u_Model;
    mat4 u_View;
    mat4 u_Proj;
    mat4 u_LightViewProj;
    vec3 u_LightDir;
    vec3 u_CameraPos;
};

layout(set=0, binding=1) uniform samplerCube u_EnvIrradiance;
layout(set=0, binding=2) uniform samplerCube u_EnvSpecular;
layout(set=0, binding=3) uniform sampler2D u_BRDF;
layout(set=0, binding=4) uniform sampler2D u_ShadowMap;

layout(set=1, binding=0) uniform sampler2D u_BaseColor;
layout(set=1, binding=1) uniform sampler2D u_MetalRoughness;
layout(set=1, binding=2) uniform sampler2D u_Normal;
layout(set=1, binding=3) uniform sampler2D u_AO;
layout(set=1, binding=4) uniform sampler2D u_Emissive;

layout(set=2, binding=0) uniform Material
{
    vec4 u_BaseColorFactor;
    float u_MetallicFactor;
    float u_RoughnessFactor;
};

const vec3 kDielectricSpecular = vec3(0.04);
const vec3 kBlack = vec3(0.0);

void vert()
{
    vec4 world = u_Model * vec4(a_Position, 1.0);

    v_UV = a_UV;

    vec3 bitangent = a_Tangent.w * cross(a_Normal, vec3(a_Tangent));

    v_Tangent   = normalize(mat3(u_Model) * vec3(a_Tangent));
    v_Bitangent = normalize(mat3(u_Model) * bitangent);
    v_Normal    = normalize(mat3(u_Model) * a_Normal);
    v_Pos = world.xyz;
    v_LightPos = u_LightViewProj * world;

    gl_Position = u_Proj * u_View * world;
}

float GGX_G2_fSpec(float NdotL, float NdotV, float a2)
{
    float i = max(NdotL, 0.0);
    float o = max(NdotV, 0.0);

    return 0.5 / (o * sqrt(a2 + i*(i-a2*i)) + i * sqrt(a2 + o*(o - a2*o)));
}

float GGX_D(vec3 m, vec3 n, float a2)
{
    float NdotM = dot(n, m);
    float denom = 1.0 + NdotM * NdotM * (a2 - 1);

    return NdotM > 0.0 ? a2 / (PI * denom * denom) : 0.0;
}

void frag()
{
    vec4 metallicRoughness = texture(u_MetalRoughness, v_UV);
    vec3 base = texture(u_BaseColor, v_UV).rgb * u_BaseColorFactor.rgb;

    vec3 t = normalize(v_Tangent);
    vec3 b = normalize(v_Bitangent);
    vec3 n = normalize(v_Normal);

    vec3 nTex = texture(u_Normal, v_UV).rgb * 2.0 - vec3(1.0);
    nTex.y = -nTex.y;
    vec3 N = mat3(t, b, n) * normalize(nTex);

    float metal = metallicRoughness.b * u_MetallicFactor;
    float rough = metallicRoughness.g * u_RoughnessFactor;

    vec3 shaded = vec3(0.0);

    vec3 albedo = mix(base * (1 - kDielectricSpecular.r), kBlack, metal);
    vec3 F0 = mix(kDielectricSpecular, base, metal);
    float a2 = rough * rough * rough * rough;

    vec3 V = normalize(u_CameraPos - v_Pos);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);

    vec3 fsRough = F0 + (max(vec3(1.0 - rough), F0) - F0) * fresnel;
    vec3 kD = 1.0 - fsRough;

    vec3 R = 2.0 * dot(V, N)*N - V;
    vec3 F = F0 + (1.0 - F0) * fresnel;

    R.y = -R.y;// TMP
    vec3 envSpecular = textureLod(u_EnvSpecular, R, rough * 5.0).rgb;

    vec2 brdf = texture(u_BRDF, vec2(NdotV, rough)).rg;

    vec3 N_adj = vec3(N.x, -N.y, N.z);// TMP
    vec3 ambient = kD * albedo * texture(u_EnvIrradiance, N_adj).rgb;
    ambient += envSpecular * (F * brdf.x + brdf.y);

    float ao = texture(u_AO, v_UV).r;
    shaded += ao * ambient;

    // Emissive
    shaded += texture(u_Emissive, v_UV).rgb;

    // Directional light:
    vec3 L = -u_LightDir;
    float NdotL = dot(N, L);
    if (NdotL > 0.0)
    {
        vec3 H = normalize(L + V);

        float fresnel_H = pow(1.0 - max(dot(H, L), 0.0), 5.0);
        vec3 F_H = F0 + (1 - F0) * fresnel_H;

        vec3 cLight = vec3(1.0);

        vec3 fL_diff = kD * (1.0/PI) * albedo;
        vec3 fL_spec = F_H * GGX_G2_fSpec(NdotL, NdotV, a2) * GGX_D(H, N, a2);

        vec3 fL = fL_diff + fL_spec;
        vec3 contrib = PI * fL * cLight * NdotL;

        // Test shadowing:
        vec4 lightPos = v_LightPos;
        vec2 lightCoords = 0.5 * lightPos.xy + vec2(0.5);

        float ref = texture(u_ShadowMap, lightCoords).r;
        float depth = exp(-C * lightPos.z);
        float shadow = depth * ref;

        // Temp bounds check
        shadow = all(lessThan(abs(lightPos.xyz), vec3(0.98))) ? shadow : 1.0;

        shadow = clamp(shadow, 0.0, 1.0);

        contrib *= shadow;
        shaded += contrib;
    }

    o_Color = vec4(shaded, 1.0);
}