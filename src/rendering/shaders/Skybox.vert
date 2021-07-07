#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec3 vBitangent;
layout(location = 4) in vec2 vUV;

layout(location = 0) out vec3 oDirection;

layout(set = 0, binding = 0) uniform MyBuffer
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos;
};

void main()
{
    oDirection = vPosition;
    vec4 pos = uProj * vec4(mat3(uView) * vPosition, 1.0);
    gl_Position = pos.xyww;
}