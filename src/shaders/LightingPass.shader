#include "VertexInput.shader"

#define PI 3.14159265358979323846

layout(location=0)

// GBuffer
layout(set=0, binding=0) uniform sampler2D u_BaseColor;
layout(set=0, binding=1) uniform sampler2D u_Normal;
layout(set=0, binding=2) uniform sampler2D u_MetalRough;
layout(set=0, binding=3) uniform sampler2D u_Depth;

layout(location=0) varying vec2 v_TexCoord;

layout(location=0) out vec4 o_Color;

layout(set=0, binding=4) uniform Globals
{
    vec4 u_ScreenToView;
};

struct DirectionalLight
{
    vec4 color;
    vec3 direction;
    mat4 proj;
    vec4 plane[3];
    vec3 scale[3];
    vec3 offset[3];
};

// Analytical lights
layout(set=1, binding=0) uniform Lights
{
    DirectionalLight u_DirectionalLight;
};

// Environment IBL
layout(set=1, binding=1) uniform samplerCube u_EnvIrradiance;
layout(set=1, binding=2) uniform samplerCube u_EnvSpecular;
layout(set=1, binding=3) uniform sampler2D u_BRDF;

layout(set=1, binding=4) uniform sampler2DArray u_ShadowMap;
layout(set=1, binding=5) uniform sampler2D u_ScreenAO;
layout(set=1, binding=6) uniform sampler2D u_ScreenReflections;

vec3 ScreenToView(vec2 coord)
{
    float depth = textureLod(u_Depth, coord, 0.0).r;
    float z = u_ScreenToView.w / (depth - u_ScreenToView.z);
    vec2 pos = u_ScreenToView.xy * (coord - vec2(0.5)) * vec2(z);

    return vec3(pos, z);
}

float GGX_G2_fSpec(float NdotL, float NdotV, float a2)
{
    float i = max(NdotL, 0.0);
    float o = max(NdotV, 0.0);

    return 0.5 / (o * sqrt(a2 + i*(i-a2*i)) + i * sqrt(a2 + o*(o - a2*o)));
}

float GGX_D(float NdotM, float a2)
{
    float denom = NdotM * NdotM * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

const float kDepthBias = 0.001;
const vec4 kMomentBiasTarget = vec4(0.0, 0.375, 0.0, 0.375);
const float kMomentBiasWeight = 0.0000003;
const float kIntensityScale = 1.02;

float Shadow(vec3 coord, float depth)
{
    float z = depth * 2.0 - 1.0;
    z -= kDepthBias;

    vec4 moments = texture(u_ShadowMap, coord);
    vec4 b = mix(moments, kMomentBiasTarget, kMomentBiasWeight);

    // Compute entries of the LDL* decomposition of the Hankel moment matrix B
    float L21xD11 = fma(-b.x, b.y, b.z);
    float D11 = fma(-b.x, b.x, b.y);
    float D22a = fma(-b.y, b.y, b.w);
    float D22xD11 = dot(vec2(D22a, -L21xD11), vec2(D11, L21xD11));
    float invD11 = 1.0 / D11;
    float L21 = L21xD11 * invD11;
    float D22 = D22xD11 * invD11;
    float invD22 = 1.0 / D22;

    vec3 c = vec3(1.0, z, z * z);
    // Solve L*c1 = z
    c.y -= b.x;
    c.z -= b.y + L21 * c.y;
    // Scale by inverse D
    c.y *= invD11;
    c.z *= invD22;
    // Solve L^T * c2 = c1 for c2
    c.y -= L21 * c.z;
    c.x -= dot(c.yz, b.xy);

    float invC2 = 1.0 / c.z;
    float p = c.y * invC2;
    float q = c.x * invC2;
    float D =(p * p * 0.25) - q;
    float r = sqrt(D);

    float z1 = -p * 0.5 - r;
    float z2 = -p * 0.5 + r;

    vec4 mask = z2 < z ? vec4(z1, z, 1.0, 1.0) :
    (z1 < z ? vec4(z, z1, 0.0, 1.0) : vec4(0.0, 0.0, 0.0, 0.0));

    float quotient = (mask.x*z2 - b.x*(mask.x + z2) +b.y) / ((z2 - mask.y)*(z -z1));
    float intensity = mask.z + mask.w * quotient;
    intensity *= kIntensityScale;

    return 1.0 - clamp(intensity, 0.0, 1.0);
}

void Vertex()
{
    v_TexCoord = a_UV;
    gl_Position = vec4(a_Position, 1.0);
}

void Fragment()
{
    vec2 coord = gl_FragCoord.xy / vec2(textureSize(u_BaseColor, 0).xy);
    vec3 pos = ScreenToView(coord);
    vec3 N = 2.0 * texture(u_Normal, coord).rgb - 1.0;
    vec3 V = normalize(-pos);
    float NdotV = dot(N, V);

    vec3 base = texture(u_BaseColor, coord).rgb;
    vec2 metalRough = texture(u_MetalRough, coord).rg;
    float metal = metalRough.x;
    float rough = metalRough.y;
    float a = rough * rough;
    float a2 = a * a;

    const vec3 kDielectricSpecular = vec3(0.04);
    const vec3 kBlack = vec3(0.0);

    vec3 albedo = mix(base * (1 - kDielectricSpecular.r), kBlack, metal);
    vec3 F0 = mix(kDielectricSpecular, base, metal);

    vec3 shaded = vec3(0.0);
    {
        // Directional light
        vec3 L = normalize(-u_DirectionalLight.direction);
        vec3 H = normalize(L + V);
        float NdotL = dot(N, L);
        float NdotH = dot(N, H);

        float fresnel_H = pow(1.0 - max(dot(H, L), 0.0), 5.0);
        vec3 F_H = F0 + (1.0 - F0) * fresnel_H;

        vec3 kD = 1.0 - F_H;

        vec3 fL_diff = kD * (1.0/PI) * albedo;
        vec3 fL_spec = F_H * GGX_G2_fSpec(NdotL, NdotV, a2) * GGX_D(NdotH, a2);

        vec3 fL = fL_diff + fL_spec;
        vec3 contrib = PI * fL * u_DirectionalLight.color.rgb * NdotL;

        // Shadowing
        vec3 lightPos0 = (u_DirectionalLight.proj * vec4(pos, 1.0)).xyz;
        vec3 lightPos1 = lightPos0 * u_DirectionalLight.scale[0] + u_DirectionalLight.offset[0];
        vec3 lightPos2 = lightPos0 * u_DirectionalLight.scale[1] + u_DirectionalLight.offset[1];
        vec3 lightPos3 = lightPos0 * u_DirectionalLight.scale[2] + u_DirectionalLight.offset[2];

        vec3 planes = vec3(
        dot(vec4(pos, 1.0), u_DirectionalLight.plane[0]),
        dot(vec4(pos, 1.0), u_DirectionalLight.plane[1]),
        dot(vec4(pos, 1.0), u_DirectionalLight.plane[2])
        );
        bool beyond2 = (planes.y >= 0.0);
        bool beyond3 = (planes.z >= 0.0);

        float layer1 = float(beyond2) * 2.0;
        float layer2 = float(beyond3) * 2.0 + 1.0;

        vec3 coord1 = beyond2 ? lightPos2 : lightPos0;
        vec3 coord2 = beyond3 ? lightPos3 : lightPos1;

        vec3 blend = clamp(planes, 0.0, 1.0);
        float weight = beyond2 ? blend.y - blend.z : 1.0 - blend.x;

        float shadow1 = Shadow(vec3(0.5 * coord1.xy + vec2(0.5), layer1), coord1.z);
        float shadow2 = Shadow(vec3(0.5 * coord2.xy + vec2(0.5), layer2), coord2.z);

        float shadow = mix(shadow2, shadow1, weight);
        contrib *= shadow;

        if (NdotL > 0.0) shaded += contrib;
    }

    {
        // Environment lighting
        vec3 R = 2.0 * dot(V, N)*N - V;
        //R.y = -R.y;// TMP
        vec3 N_adj = vec3(N.x, -N.y, N.z);// TMP

        vec2 brdf = texture(u_BRDF, vec2(NdotV, rough)).rg;
        vec3 envSpecular = textureLod(u_EnvSpecular, R, rough * 5.0).rgb;
        vec3 ssrSpecular = texture(u_ScreenReflections, coord).rgb;

        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
        vec3 fsRough = F0 + (max(vec3(1.0 - rough), F0) - F0) * fresnel;
        vec3 F = F0 + (1.0 - F0) * fresnel;
        vec3 kD = 1.0 - fsRough;

        vec3 ambient = kD * albedo * texture(u_EnvIrradiance, N_adj).rgb;
        ambient += envSpecular * (F * brdf.x + brdf.y);

        float ao = texture(u_ScreenAO, coord).r;

        shaded += ao * ambient;
    }

    o_Color = vec4(shaded, 1.0);
}
