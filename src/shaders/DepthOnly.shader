#include "shared/VertexLayout"

layout(set=0, binding=0) uniform Globals
{
    mat4 u_MVP;
};

void vert()
{
    gl_Position = u_MVP * vec4(a_Position.xyz, 1.0);
}