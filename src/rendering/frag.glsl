#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 oUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D sBaseColor;

void main()
{
    outColor = vec4(texture(sBaseColor, oUV).rgb, 1.0);
}