#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 iDirection;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 1) uniform samplerCube sEnvIrradiance;
layout(set = 0, binding = 2) uniform samplerCube sEnvSpecular;
layout(set = 0, binding = 3) uniform sampler2D sBRDF;
layout(set = 1, binding = 0) uniform samplerCube sEnvironment;

void main()
{
    vec3 dir = -normalize(iDirection);
    dir.x = -dir.x;
    vec3 col = texture(sEnvironment, dir).rgb;
    oColor = vec4(col, 1.0);
}