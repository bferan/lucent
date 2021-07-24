#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 iUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sFontTex;

void main()
{
    float value = texture(sFontTex, iUV).r;
    outColor = vec4(value);
}