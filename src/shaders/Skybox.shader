#include "Core.shader"
#include "VertexInput.shader"
#include "View.shader"

layout(location=0) varying vec3 v_Direction;

layout(location=0) out vec4 o_Color;

layout(set=1, binding=0) uniform samplerCube u_Skybox;

void Vertex()
{
    v_Direction = a_Position;
    vec4 pos = u_ViewToScreen * vec4(mat3(u_WorldToView) * a_Position, 1.0);
    gl_Position = pos.xyww;
}

void Fragment()
{
    vec3 dir = -normalize(v_Direction);
    dir.x = -dir.x;
    vec3 col = texture(u_Skybox, dir).rgb;

    o_Color = vec4(col, 1.0);
}