#include "Core.shader"

vec3 SampleGGX(vec3 V, float alpha, vec2 rand)
{
    // Sampling the GGX Distribution of Visible Normals [Heitz2018]
    // http://jcgt.org/published/0007/04/01/

    vec3 Vh = normalize(vec3(V.xy * alpha, V.z));

    // Construct axes of disk orthogonal to view direction
    float len2 = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = len2 > 0.0 ? inversesqrt(len2) * vec3(-Vh.y, Vh.x, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T2 = cross(Vh, T1);

    // Sample random point on disk
    float radius = sqrt(rand.x);
    float phi = 2.0 * PI * rand.y;
    float t1 = radius * cos(phi);
    float t2 = radius * sin(phi);

    // Compress vertical coordinate proportional to projection of lower half disk
    float rescale = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - rescale) * sqrt(1.0 - t1 * t1) + rescale * t2;

    // Reproject back onto hemisphere
    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(1.0 - t1 * t1 - t2 * t2, 0.0)) * Vh;

    vec3 N = normalize(vec3(Nh.xy * alpha, max(Nh.z, 0.0)));
    return N;
}

// Generate a tangent-space sample biased toward a GGX lobe
vec3 SampleBiasedGGX(vec2 rand, float roughness)
{
    float a = roughness * roughness;

    float phi = rand.x * 2.0 * PI;
    float cosTheta = sqrt((1.0 - rand.y)/(1.0 + (a*a - 1.0)*rand.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    return vec3(sinTheta*cos(phi), sinTheta*sin(phi), cosTheta);
}

vec2 Hammersley(uint i, uint n)
{
    // Reverse bits to generate Van der Corput value
    uint bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float v = float(bits) * 2.3283064365386963e-10;// / 0x100000000
    return vec2(float(i)/float(n), v);
}

float GGX_D(float NdotM, float a2)
{
    float denom = NdotM * NdotM * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GGX_SmithG1(float NdotV, float a2)
{
    float denom = NdotV + sqrt(a2 + (1.0 - a2) * NdotV * NdotV);
    return 2.0 * NdotV / denom;
}

// Combined G2 and specular BRDF term
float GGX_G2_fSpec(float NdotL, float NdotV, float a2)
{
    a2 = max(a2, 0.0001);

    float i = max(NdotL, 0.0);
    float o = max(NdotV, 0.0);

    return 0.5 / (o * sqrt(a2 + i*(i-a2*i)) + i * sqrt(a2 + o*(o - a2*o)));
}

float GGX_SmithCorrelatedG2(float NdotL, float NdotV, float a2)
{
    float NdotV2 = NdotV * NdotV;
    float NdotL2 = NdotL * NdotL;

    float lambdaV = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotV2))/NdotV2)) * 0.5;
    float lambdaL = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotL2))/NdotL2)) * 0.5;
    return 1.0 / (1.0 + lambdaV + lambdaL);
}

float SchlickFresnel(float NdotV)
{
    return pow(1.0 - NdotV, 5.0);
}