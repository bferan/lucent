#include "VertexInput.shader"

layout(set=0, binding=0) uniform Globals
{
    mat4 u_MVP;
};

void Vertex()
{
    gl_Position = u_MVP * vec4(a_Position.xyz, 1.0);
}