#include "shared/VertexLayout"
#include "shared/PBR"

layout(location=0) varying Vertex
{
    vec2 v_UV;
    vec3 v_Tangent;
    vec3 v_Bitangent;
    vec3 v_Normal;
    vec3 v_Pos;
    vec4 v_LightPos;
    vec3 v_LightPlanes;
};

layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform Globals
{
    mat4 u_Model;
    mat4 u_View;
    mat4 u_Proj;
    mat4 u_LightViewProj;
    mat4 u_LightViewProj1;
    vec3 u_LightDir;
    vec4 u_LightPlane[3];
    vec3 u_LightScale[3];
    vec3 u_LightOffset[3];
    vec3 u_CameraPos;
};

layout(set=0, binding=1) uniform samplerCube u_EnvIrradiance;
layout(set=0, binding=2) uniform samplerCube u_EnvSpecular;
layout(set=0, binding=3) uniform sampler2D u_BRDF;
layout(set=0, binding=4) uniform sampler2DArray u_ShadowMap;

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

    v_LightPlanes = vec3(
    dot(world, u_LightPlane[0]),
    dot(world, u_LightPlane[1]),
    dot(world, u_LightPlane[2])
    );

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

const float depthBias = 0.0;
const vec4 momentBiasTarget = vec4(0.0, 0.375, 0.0, 0.375);
const float momentBiasWeight = 0.0000003;

float getShadow(vec3 coord, float depth)
{
    float z = depth * 2.0 - 1.0;
    z -= depthBias;

    vec4 moments = texture(u_ShadowMap, coord);
    vec4 b = mix(moments, momentBiasTarget, momentBiasWeight);

    // Compute entries of the LDL* decomposition of the Hankel matrix B
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

    vec4 s = z2 < z ? vec4(z1, z, 1.0, 1.0) :
        (z1 < z ? vec4(z, z1, 0.0, 1.0) : vec4(0.0, 0.0, 0.0, 0.0));

    float quotient = (s.x*z2 - b.x*(s.x + z2) +b.y) / ((z2 - s.y)*(z -z1));
    float intensity = s.z + s.w * quotient;
    intensity *= 1.02;

    return 1.0 - clamp(intensity, 0.0, 1.0);
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
        vec3 lightPos0 = v_LightPos.xyz;
        vec3 lightPos1 = lightPos0 * u_LightScale[0] + u_LightOffset[0];
        vec3 lightPos2 = lightPos0 * u_LightScale[1] + u_LightOffset[1];
        vec3 lightPos3 = lightPos0 * u_LightScale[2] + u_LightOffset[2];

        vec3 planes = v_LightPlanes;
        bool beyond2 = (planes.y >= 0.0);
        bool beyond3 = (planes.z >= 0.0);

        float layer1 = float(beyond2) * 2.0;
        float layer2 = float(beyond3) * 2.0 + 1.0;

        vec3 coord1 = beyond2 ? lightPos2 : lightPos0;
        vec3 coord2 = beyond3 ? lightPos3 : lightPos1;

        vec3 blend = clamp(planes, 0.0, 1.0);
        float weight = beyond2 ? blend.y - blend.z : 1.0 - blend.x;

        float shadow1 = getShadow(vec3(0.5 * coord1.xy + vec2(0.5), layer1), coord1.z);
        float shadow2 = getShadow(vec3(0.5 * coord2.xy + vec2(0.5), layer2), coord2.z);

        float shadow = mix(shadow2, shadow1, weight);

        // Temp bounds check
        //shadow = all(lessThan(abs(lightPos.xy), vec2(0.98))) ? shadow : 1.0;

        contrib *= shadow;
        shaded += contrib;
    }

    o_Color = vec4(shaded, 1.0);
}