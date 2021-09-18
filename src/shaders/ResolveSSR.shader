#include "shared/PBR"

layout(set=0, binding=0) uniform sampler2D u_PrevColor;
layout(set=0, binding=1) uniform sampler2D u_Rays;
layout(set=0, binding=2) uniform sampler2D u_Depth;
layout(set=0, binding=3) uniform sampler2D u_BaseColor;
layout(set=0, binding=4) uniform sampler2D u_Normals;
layout(set=0, binding=5) uniform sampler2D u_MetalRoughness;

layout(set=0, binding=6, rgba32f) uniform image2D u_Result;

layout(set=0, binding=7) uniform Globals
{
    vec4 u_ScreenToView;
    mat4 u_Proj;
};

float GGX_D(float NdotM, float a2)
{
    float denom = NdotM * (NdotM * a2 - NdotM) + 1.0;
    return a2 / (PI * denom * denom);
}

float GGX_SmithG1(float NdotV, float a2)
{
    float denom = NdotV + sqrt(a2 + (1.0 - a2) * NdotV * NdotV);
    return 2.0 * NdotV / denom;
}

float GGX_SmithCorrelatedG2(float NdotL, float NdotV, float a2)
{
    float NdotV2 = NdotV * NdotV;
    float NdotL2 = NdotL * NdotL;

    float lambdaV = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotV2))/NdotV2)) * 0.5;
    float lambdaL = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotL2))/NdotL2)) * 0.5;
    return 1.0 / (1.0 + lambdaV + lambdaL);
}

float GGX_G2_fSpec(float NdotL, float NdotV, float a2)
{
    float i = max(NdotL, 0.0);
    float o = max(NdotV, 0.0);

    return 0.5 / (o * sqrt(a2 + i*(i-a2*i)) + i * sqrt(a2 + o*(o - a2*o)));
}

const vec3 kDielectricSpecular = vec3(0.04);
const vec3 kBlack = vec3(0.0);

vec3 screenToView(vec2 coord)
{
    float depth = textureLod(u_Depth, coord, 0.0).r;
    float z = u_ScreenToView.w / (depth - u_ScreenToView.z);
    vec2 pos = u_ScreenToView.xy * (coord - vec2(0.5)) * vec2(z);

    return vec3(pos, z);
}

void compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = (vec2(imgCoord) + vec2(0.5)) / vec2(imageSize(u_Result).xy);

    // Local brdf terms
    vec3 pos = screenToView(coord);
    vec3 V = normalize(-pos);

    vec3 baseColor = texture(u_BaseColor, coord).rgb;
    vec2 metalRough = texture(u_MetalRoughness, coord).rg;

    float rough = metalRough.y;
    float alpha = rough * rough;
    float alpha2 = alpha * alpha;

    vec3 F0 = mix(kDielectricSpecular, baseColor, metalRough.x);

    vec3 N = normalize(texture(u_Normals, coord).xyz);

    // For each neighboring pixel
    vec3 result = vec3(0.0);
    vec3 totalWeight = vec3(0.0);

    for (int dx = 0; dx <= 0; ++dx)
    {
        for (int dy = 0; dy <= 0; ++dy)
        {
            ivec2 rayCoord = imgCoord + ivec2(dx, dy);
            vec4 ray = texelFetch(u_Rays, rayCoord, 0);

            if (ray.z == 0.0)
                continue;

            vec3 color = texture(u_PrevColor, ray.xy).rgb;

            // brdf = F * D * G2 / 4 NdotV * NdotL
            // brdf = F * D * GGXG2FSPEC
            // integrand = brdf * NdotL

            // Evaluate local brdf
            vec3 reflectedPos = screenToView(ray.xy);
            vec3 L = normalize(reflectedPos - pos);
            vec3 H = normalize(V + L);

            float fresnel_H = pow(1.0 - max(dot(H, L), 0.0), 5.0);
            vec3 F_H = F0 + (1 - F0) * fresnel_H;

            float NdotL = dot(N,L);
            float NdotV = dot(N,V);

            vec3 brdf = F_H * GGX_G2_fSpec(NdotL, NdotV, alpha2) * GGX_D(dot(N,H), alpha2);
            vec3 integrand = brdf * NdotL;
            vec3 weight = integrand / ray.z;

            result += weight * color;
            totalWeight += weight;
        }
    }
    result /= totalWeight;

    imageStore(u_Result, imgCoord, vec4(result, 1.0));
}