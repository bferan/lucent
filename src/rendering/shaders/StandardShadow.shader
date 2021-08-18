#include "shared/VertexLayout"

const float C = 80.0;

layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform Globals
{
    mat4 u_MVP;
};

layout(location=0) varying Vertex
{
    vec4 v_Pos;
};

void vert()
{
    vec4 pos = u_MVP * vec4(a_Position, 1.0);
    v_Pos = pos;
    gl_Position = pos;
}

void frag()
{
    float term = exp(C * v_Pos.z);
    o_Color = vec4(term, 0.0, 0.0, 1.0);
}
