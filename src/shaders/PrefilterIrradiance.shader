#include "VertexInput"

layout(location=0) varying vec3 v_Direction;
layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform Globals
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Roughness;
};
layout(set=0, binding=1) uniform samplerCube u_EnvCube;

#define PI 3.14159265358979323846

vec3 Irradiance(vec3 dir)
{
    vec3 col = texture(u_EnvCube, dir).rgb;
    col = clamp(col, vec3(0.0), vec3(20.0));
    //col = col / (col + vec3(1.0)); // Tone map HDR
    return col;
}

void Vertex()
{
    v_Direction = a_Position;
    vec4 pos = u_Proj * vec4(mat3(u_View) * a_Position, 1.0);
    gl_Position = pos.xyww;
}

void Fragment()
{
    vec3 N = normalize(v_Direction);
    vec3 T1 = cross(N, vec3(0.0, 1.0, 0.0));
    vec3 T2 = cross(N, T1);

    vec3 irradiance = vec3(0.0);
    float sampleCount = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += 0.01)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += 0.01)
        {
            vec3 local = vec3(cos(phi)*sin(theta), sin(phi)*sin(theta), cos(theta));
            vec3 world = local.x * T1 + local.y * T2 + local.z * N;
            irradiance += Irradiance(world) * cos(theta) * sin(theta);
            sampleCount++;
        }
    }
    irradiance *= PI/sampleCount;

    o_Color = vec4(irradiance, 1.0);
}

