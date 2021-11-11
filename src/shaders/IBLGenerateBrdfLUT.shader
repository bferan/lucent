#include "Core.shader"
#include "VertexInput.shader"
#include "PBR.shader"

layout(location=0) varying vec2 v_TC;

layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform Globals
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Roughness;
};
layout(set=0, binding=1) uniform samplerCube u_EnvCube;

void Vertex()
{
    v_TC = a_UV;
    gl_Position = vec4(a_Position, 1.0);
}

uint kSampleCount = 1024u;

// Generate a lookup table of pre-integrated BRDF values
void Fragment()
{
    float NdotV = v_TC.x;
    float roughness = v_TC.y;
    float a = roughness * roughness;
    float a2 = a * a;

    vec3 V = vec3(sqrt(1.0 - NdotV*NdotV), 0.0, NdotV);

    float scale = 0.0;
    float bias = 0.0;

    for (uint i = 0; i < kSampleCount; ++i)
    {
        vec3 H = SampleBiasedGGX(Hammersley(i, kSampleCount), roughness);
        vec3 L = 2.0 * dot(V, H)*H - V;

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G2 = GGX_SmithCorrelatedG2(NdotL, NdotV, a2);

            // Inbound light: NdotL
            // BRDF: D*F*G2 / (4*NdotL*NdotV)
            // Divide by PDF (D * NdotH / (4 * VdotH))
            float G = (G2 * VdotH) / (NdotH * NdotV);

            float power = pow(1.0 - VdotH, 5);
            scale += G * (1.0 - power);
            bias  += G * power;
        }
    }
    scale /= kSampleCount;
    bias /= kSampleCount;

    o_Color = vec4(scale, bias, 0.0, 1.0);
}