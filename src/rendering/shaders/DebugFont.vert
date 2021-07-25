#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec3 vBitangent;
layout(location = 4) in vec2 vUV;
layout(location = 5) in vec4 vColor;

layout(location = 0) out vec2 iUV;
layout(location = 1) out vec4 iColor;

void main()
{
    iUV = vUV;
    iColor = vColor;
    gl_Position = vec4(vPosition, 1.0);
}