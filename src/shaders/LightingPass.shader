#include "Core.shader"
#include "View.shader"
#include "VertexInput.shader"
#include "MomentShadow.shader"
#include "PBR.shader"

layout(location=0) varying vec2 v_TexCoord;
layout(location=0) out vec4 o_Color;

// GBuffer
layout(set=1, binding=0) uniform sampler2D u_BaseColor;
layout(set=1, binding=1) uniform sampler2D u_Normal;
layout(set=1, binding=2) uniform sampler2D u_MetalRough;
layout(set=1, binding=3) uniform sampler2D u_Depth;
layout(set=1, binding=4) uniform sampler2D u_Emissive;

// Analytical lights
struct DirectionalLight
{
    vec4 color;
    vec3 direction;
    mat4 proj;
    vec4 plane[3];
    vec3 scale[3];
    vec3 offset[3];
};

struct PointLight
{
    vec4 color;
    vec3 position;
};

layout(set=2, binding=0) uniform Lights
{
    DirectionalLight u_DirectionalLight;
};

// Environment IBL
const float kMaxEnvSpecularMips = 5.0;

layout(set=2, binding=1) uniform samplerCube u_EnvIrradiance;
layout(set=2, binding=2) uniform samplerCube u_EnvSpecular;
layout(set=2, binding=3) uniform sampler2D u_BRDF;

layout(set=2, binding=4) uniform sampler2DArray u_ShadowMap;
layout(set=2, binding=5) uniform sampler2D u_ScreenAO;
layout(set=2, binding=6) uniform sampler2D u_ScreenReflections;

// Compute shadow intensity from view-space position
float GetDirectionalShadow(vec3 pos)
{
    vec3 lightPos0 = (u_DirectionalLight.proj * vec4(pos, 1.0)).xyz;
    vec3 lightPos1 = lightPos0 * u_DirectionalLight.scale[0] + u_DirectionalLight.offset[0];
    vec3 lightPos2 = lightPos0 * u_DirectionalLight.scale[1] + u_DirectionalLight.offset[1];
    vec3 lightPos3 = lightPos0 * u_DirectionalLight.scale[2] + u_DirectionalLight.offset[2];

    vec3 planes = vec3(
        dot(vec4(pos, 1.0), u_DirectionalLight.plane[0]),
        dot(vec4(pos, 1.0), u_DirectionalLight.plane[1]),
        dot(vec4(pos, 1.0), u_DirectionalLight.plane[2]));

    bool beyond2 = (planes.y >= 0.0);
    bool beyond3 = (planes.z >= 0.0);

    float layer1 = float(beyond2) * 2.0;
    float layer2 = float(beyond3) * 2.0 + 1.0;

    vec3 coord1 = beyond2 ? lightPos2 : lightPos0;
    vec3 coord2 = beyond3 ? lightPos3 : lightPos1;

    // Determine blend factor to smoothly fade between adjacent map cascades
    vec3 blend = clamp(planes, 0.0, 1.0);
    float weight = beyond2 ? blend.y - blend.z : 1.0 - blend.x;

    vec4 moments1 = texture(u_ShadowMap, vec3(0.5 * coord1.xy + vec2(0.5), layer1));
    vec4 moments2 = texture(u_ShadowMap, vec3(0.5 * coord2.xy + vec2(0.5), layer2));

    float shadow1 = CalculateMomentShadow(moments1, coord1.z);
    float shadow2 = CalculateMomentShadow(moments2, coord2.z);

    return mix(shadow2, shadow1, weight);
}

// Compute lighting received from the main directional light
vec3 GetDirectionalLight(vec3 pos, vec3 N, vec3 V, float a2, vec3 F0, vec3 albedo)
{
    vec3 L = normalize(-u_DirectionalLight.direction);
    vec3 H = normalize(L + V);

    float NdotL = dot(N, L);
    float NdotH = dot(N, H);
    float NdotV = dot(N, V);

    float fresnel_H = SchlickFresnel(max(dot(H, L), 0.0));
    vec3 F_H = F0 + (1.0 - F0) * fresnel_H;

    vec3 kD = 1.0 - F_H;

    vec3 fL_diff = kD * (1.0/PI) * albedo;
    vec3 fL_spec = F_H * GGX_G2_fSpec(NdotL, NdotV, a2) * GGX_D(NdotH, a2);

    vec3 fL = fL_diff + fL_spec;
    vec3 contrib = PI * fL * u_DirectionalLight.color.rgb * NdotL;

    // Shadowing
    contrib *= GetDirectionalShadow(pos);

    return NdotL > 0.0 ? contrib : vec3(0.0);
}

// Compute ambient lighting from (view space) direction
vec3 GetEnvironmentLight(vec3 N, vec3 V, vec2 screenCoord, float rough, vec3 F0, vec3 albedo)
{
    vec3 Vworld = mat3(u_ViewToWorld) * V;
    vec3 Nworld = mat3(u_ViewToWorld) * N;
    Vworld.y = -Vworld.y;
    Nworld.y = -Nworld.y;
    vec3 Rworld = 2.0 * dot(Vworld, Nworld) * Nworld - Vworld;

    float NdotV = dot(N, V);
    vec2 brdf = texture(u_BRDF, vec2(NdotV, rough)).rg;

    // Determine specular contribution (blend between env probe and SSR)
    vec3 envSpecular = textureLod(u_EnvSpecular, Rworld, rough * kMaxEnvSpecularMips).rgb;
    vec4 ssrSpecular = texture(u_ScreenReflections, screenCoord).rgba;

    float ssrMix = clamp(ssrSpecular.a, 0.0, 1.0);
    vec3 specular = mix(envSpecular, ssrSpecular.rgb, ssrMix);

    float fresnel = SchlickFresnel(max(dot(Nworld, Vworld), 0.0));

    vec3 fsRough = F0 + (max(vec3(1.0 - rough), F0) - F0) * fresnel;
    vec3 F = F0 + (1.0 - F0) * fresnel;
    vec3 kD = 1.0 - fsRough;

    vec3 ambient = kD * albedo * texture(u_EnvIrradiance, Nworld).rgb;
    ambient += specular * (F * brdf.x + brdf.y);

    float ao = texture(u_ScreenAO, screenCoord).r;
    return ao * ambient;
}

void Vertex()
{
    v_TexCoord = a_UV;
    gl_Position = vec4(a_Position, 1.0);
}

void Fragment()
{
    // Extract view space directions
    vec2 coord = gl_FragCoord.xy / vec2(textureSize(u_BaseColor, 0).xy);
    vec3 pos = ScreenToView(coord, textureLod(u_Depth, coord, 0).r);
    vec3 N = 2.0 * texture(u_Normal, coord).rgb - 1.0;
    vec3 V = normalize(-pos);
    float NdotV = dot(N, V);

    // Extract material parameters
    vec3 base = texture(u_BaseColor, coord).rgb;
    vec2 metalRough = texture(u_MetalRough, coord).rg;
    float metal = metalRough.x;
    float rough = metalRough.y;
    float a = rough * rough;
    float a2 = a * a;

    vec3 albedo = mix(base * (1 - kDielectricSpecular.r), kBlack, metal);
    vec3 F0 = mix(kDielectricSpecular, base, metal);

    vec3 shaded = vec3(0.0);

    shaded += GetDirectionalLight(pos, N, V, a2, F0, albedo);

    shaded += GetEnvironmentLight(N, V, coord, rough, F0, albedo);

    // Emissive
    shaded += texture(u_Emissive, coord).rgb;

    o_Color = vec4(shaded, 1.0);
}
