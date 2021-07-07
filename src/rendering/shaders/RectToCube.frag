#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265358979323846

layout(location = 0) in vec3 iDirection;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 1) uniform sampler2D sRectTex;

void main()
{
    vec3 dir = normalize(iDirection);
    float lon = atan(dir.x, -dir.z)/(2*PI) + .5;
    float lat = asin(dir.y)/PI + .5;

    vec3 col = texture(sRectTex, vec2(lon, lat)).rgb;
    oColor = vec4(col, 1.0);
}