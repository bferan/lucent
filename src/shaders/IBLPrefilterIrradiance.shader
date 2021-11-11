#include "IBLCommon.shader"

layout(set=0, binding=1) uniform samplerCube u_EnvCube;

vec3 Radiance(vec3 dir)
{
    vec3 color = texture(u_EnvCube, dir).rgb;
    color = clamp(color, vec3(0.0), kMaxRadiance);
    return color;
}

void Vertex()
{
    v_Direction = a_Position;
    gl_Position = (u_Proj * vec4(mat3(u_View) * a_Position, 1.0)).xyww;
}

// Numerically integrate hemisphere of incoming radiance for each direction
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
            irradiance += Radiance(world) * cos(theta) * sin(theta);
            sampleCount++;
        }
    }
    irradiance *= PI/sampleCount;

    o_Color = vec4(irradiance, 1.0);
}