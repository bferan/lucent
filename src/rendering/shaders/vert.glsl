#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec3 vBitangent;
layout(location = 4) in vec2 vUV;

layout(location = 0) out vec2 oUV;
layout(location = 1) out vec3 oTangent;
layout(location = 2) out vec3 oBitangent;
layout(location = 3) out vec3 oNormal;
layout(location = 4) out vec3 oPos;

layout(set = 0, binding = 0) uniform MyBuffer
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos;
};

void main()
{
    vec4 world = uModel * vec4(vPosition, 1.0);

    oUV = vUV;
    oTangent   = normalize(mat3(uModel) * vTangent);
    oBitangent = normalize(mat3(uModel) * vBitangent);
    oNormal    = normalize(mat3(uModel) * vNormal);
    oPos = world.xyz;

    gl_Position = uProj * uView * world;
}