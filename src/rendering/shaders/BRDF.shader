#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265358979323846

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

// Generate a tangent-space sample biased toward a GGX lobe
vec3 SampleBiasedGGX(vec2 rand, float roughness)
{
    float a = roughness * roughness;

    float phi = rand.x * 2.0 * PI;
    float cosTheta = sqrt((1.0 - rand.y)/(1.0 + (a*a - 1.0)*rand.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    return vec3(sinTheta*cos(phi), sinTheta*sin(phi), cosTheta);
}

float SmithCorrelatedGGX(float NdotL, float NdotV, float a)
{
    float a2 = a * a;
    float NdotV2 = NdotV * NdotV;
    float NdotL2 = NdotL * NdotL;

    float lambdaV = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotV2))/NdotV2)) * 0.5;
    float lambdaL = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotL2))/NdotL2)) * 0.5;
    return 1.0 / (1.0 + lambdaV + lambdaL);
}

layout(location = 0) attribute vec3 a_Position;
layout(location = 1) attribute vec3 a_Normal;
layout(location = 2) attribute vec3 a_Tangent;
layout(location = 3) attribute vec3 a_Bitangent;
layout(location = 4) attribute vec2 a_UV;

layout(location = 0) varying vec2 v_TC;

layout(location = 0) out vec4 o_Color;

layout(set = 0, binding = 0) uniform MyBuffer
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Roughness;
};
layout(set = 0, binding = 1) uniform samplerCube u_EnvCube;

void vert()
{
    v_TC = a_UV;
    gl_Position = vec4(a_Position, 1.0);
}

void frag()
{
    float NdotV = v_TC.x;
    float roughness = v_TC.y;

    vec3 V = vec3(sqrt(1.0 - NdotV*NdotV), 0.0, NdotV);

    float scale = 0.0;
    float bias = 0.0;

    uint sampleCount = 1024u;
    for (uint i = 0; i < sampleCount; ++i)
    {
        vec3 H = SampleBiasedGGX(Hammersley(i, sampleCount), roughness);
        vec3 L = 2.0 * dot(V, H)*H - V;

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G2 = SmithCorrelatedGGX(NdotL, NdotV, roughness*roughness);

            // Inbound light: NdotL
            // BRDF: D*F*G2 / (4*NdotL*NdotV)
            // Divide by PDF (D * NdotH / (4 * VdotH))
            float G = (G2 * VdotH) / (NdotH * NdotV);

            float power = pow(1.0 - VdotH, 5);
            scale += G * (1.0 - power);
            bias  += G * power;
        }
    }
    scale /= sampleCount;
    bias /= sampleCount;

    o_Color = vec4(scale, bias, 0.0, 1.0);
}