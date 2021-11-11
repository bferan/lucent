#include "IBLCommon.shader"

layout(set=0, binding=1) uniform sampler2D u_RectTex;

void Vertex()
{
    v_Direction = a_Position;
    gl_Position = (u_Proj * vec4(mat3(u_View) * a_Position, 1.0)).xyww;
}

// Turn equirectangular projection into cube map
void Fragment()
{
    vec3 direction = normalize(v_Direction);
    float lon = atan(direction.x, -direction.z)/(2.0 * PI) + 0.5;
    float lat = asin(direction.y)/PI + 0.5;

    vec3 color = texture(u_RectTex, vec2(lon, lat)).rgb;
    o_Color = vec4(color, 1.0);
}