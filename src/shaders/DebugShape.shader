#include "shared/VertexLayout"

layout(set=0, binding=0) uniform Globals
{
    vec4 u_Color;
    mat4 u_MVP;
};

layout(location=0) out vec4 o_Color;

void vert()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}

void frag()
{
    o_Color = u_Color;
}