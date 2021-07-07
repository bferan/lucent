#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265358979323846

layout(location = 0) in vec3 iDirection;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform MyBuffer
{
    mat4 uView;
    mat4 uProj;
    float uRoughness;
};
layout(set = 0, binding = 1) uniform samplerCube sEnvCube;

vec2 Hammersley(uint i, uint n)
{
    // Reverse bits to generate Van der Corput value
    uint bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float v = float(bits) * 2.3283064365386963e-10; // / 0x100000000
    return vec2(float(i)/float(n), v);
}

// Generate a tangent-space sample biased around a GGX lobe
vec3 SampleBiasedGGX(vec2 rand, float roughness)
{
    float a = roughness * roughness;

    float phi = rand.x * 2.0 * PI;
    float cosTheta = sqrt((1.0 - rand.y)/(1.0 + (a*a - 1.0)*rand.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    return vec3(sinTheta*cos(phi), sinTheta*sin(phi), cosTheta);
}

void main()
{
    vec3 N = normalize(iDirection);
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 T1 = normalize(cross(up, N));
    vec3 T2 = cross(N, T1);

    vec3 filtered = vec3(0.0);
    float weight = 0.0;

    const uint sampleCount = 8096;
    for (uint i = 0; i < sampleCount; ++i)
    {
        vec3 local = SampleBiasedGGX(Hammersley(i, sampleCount), uRoughness);
        vec3 H = normalize(local.x * T1 + local.y * T2 + local.z * N);
        vec3 L = normalize(2.0*dot(N, H)* H - N);

        float NdotL = dot(N, L);
        if (NdotL > 0.0)
        {
            filtered += min(texture(sEnvCube, L).rgb, vec3(20.0f)) * NdotL;
            weight += NdotL;
        }
    }
    oColor = vec4(filtered/weight, 1.0);
}