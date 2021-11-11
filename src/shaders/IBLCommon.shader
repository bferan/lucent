#include "Core.shader"
#include "VertexInput.shader"

layout(location=0) varying vec3 v_Direction;
layout(location=0) out vec4 o_Color;

layout(set=0, binding=0) uniform Globals
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Roughness;
};

const vec3 kMaxRadiance = vec3(20.0);
