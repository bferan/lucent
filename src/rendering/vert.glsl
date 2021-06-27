#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vUV;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform MyBuffer
{
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
    vec3 uCol;
};

layout(set = 0, binding = 1) uniform sampler2D sBaseColor;

void main()
{
    gl_Position = uProj * uView * uModel * vec4(vPosition, 1.0);

    fragColor = texture(sBaseColor, vUV).rgb;
    //fragColor = vec3(vUV * sign(vUV), 0.0);
}