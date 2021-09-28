#include "VertexInput"

layout(location=0) varying vec2 v_UV;
layout(location=1) varying vec4 v_Color;

layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform sampler2D u_FontAtlas;

void Vertex()
{
    v_UV = a_UV;
    v_Color = a_Color;
    vec4 pos = vec4(a_Position, 1.0);
    pos.y = -pos.y;
    gl_Position = pos;
}

void Fragment()
{
    float value = texture(u_FontAtlas, v_UV).r;
    o_Color = value * v_Color;
}
