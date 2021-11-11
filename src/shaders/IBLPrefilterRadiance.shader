#include "IBLCommon.shader"
#include "PBR.shader"

layout(set=0, binding=1) uniform samplerCube u_EnvCube;

const uint kSampleCount = 8096;

void Vertex()
{
    v_Direction = a_Position;
    gl_Position = (u_Proj * vec4(mat3(u_View) * a_Position, 1.0)).xyww;
}

// Importance sample GGX lobe for a given roughness
void Fragment()
{
    vec3 N = normalize(v_Direction);
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 T1 = normalize(cross(up, N));
    vec3 T2 = cross(N, T1);

    vec3 filtered = vec3(0.0);
    float weight = 0.0;

    for (uint i = 0; i < kSampleCount; ++i)
    {
        vec3 local = SampleBiasedGGX(Hammersley(i, kSampleCount), u_Roughness);
        vec3 H = normalize(local.x * T1 + local.y * T2 + local.z * N);
        vec3 L = normalize(2.0 * dot(N, H)* H - N);

        float NdotL = dot(N, L);
        if (NdotL > 0.0)
        {
            filtered += min(texture(u_EnvCube, L).rgb, kMaxRadiance) * NdotL;
            weight += NdotL;
        }
    }
    o_Color = vec4(filtered/weight, 1.0);
}
