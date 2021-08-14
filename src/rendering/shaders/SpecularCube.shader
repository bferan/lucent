
#include "shared/VertexLayout"
#include "shared/PBR"

layout(location = 0) varying vec3 v_Direction;

layout(location = 0) out vec4 o_Color;

layout(set = 0, binding = 0) uniform Global
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Roughness;
};
layout(set = 0, binding = 1) uniform samplerCube u_EnvCube;

void vert()
{
    v_Direction = a_Position;
    vec4 pos = u_Proj * vec4(mat3(u_View) * a_Position, 1.0);
    gl_Position = pos.xyww;
}

void frag()
{
    vec3 N = normalize(v_Direction);
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 T1 = normalize(cross(up, N));
    vec3 T2 = cross(N, T1);

    vec3 filtered = vec3(0.0);
    float weight = 0.0;

    const uint sampleCount = 8096;
    for (uint i = 0; i < sampleCount; ++i)
    {
        vec3 local = SampleBiasedGGX(Hammersley(i, sampleCount), u_Roughness);
        vec3 H = normalize(local.x * T1 + local.y * T2 + local.z * N);
        vec3 L = normalize(2.0*dot(N, H)* H - N);

        float NdotL = dot(N, L);
        if (NdotL > 0.0)
        {
            filtered += min(texture(u_EnvCube, L).rgb, vec3(20.0f)) * NdotL;
            weight += NdotL;
        }
    }
    o_Color = vec4(filtered/weight, 1.0);
}
