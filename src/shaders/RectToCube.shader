#include "VertexInput"

layout(location=0) varying vec3 v_Direction;
layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform Globals
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Roughness;
};
layout(set=0, binding=1) uniform sampler2D u_RectTex;

#define PI 3.14159265358979323846

void Vertex()
{
    v_Direction = a_Position;
    vec4 pos = u_Proj * vec4(mat3(u_View) * a_Position, 1.0);
    gl_Position = pos.xyww;
}

void Fragment()
{
    vec3 dir = normalize(v_Direction);
    float lon = atan(dir.x, -dir.z)/(2*PI) + .5;
    float lat = asin(dir.y)/PI + .5;

    vec3 col = texture(u_RectTex, vec2(lon, lat)).rgb;
    o_Color = vec4(col, 1.0);
}

