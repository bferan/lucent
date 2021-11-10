#include "Core.shader"

layout(set=0, binding=0) uniform sampler2D u_Input;
layout(set=0, binding=1, rgba32f) uniform image2D u_Output;

layout(local_size_x=8, local_size_y=8) in;

#ifdef BLUR_HORIZONTAL

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = (vec2(imgCoord) + vec2(0.5)) / vec2(imageSize(u_Output).xy);

    vec3 value = vec3(0.0);
    value += 0.001 * textureOffset(u_Input, coord, ivec2(-3, 0)).rgb;
    value += 0.028 * textureOffset(u_Input, coord, ivec2(-2, 0)).rgb;
    value += 0.233 * textureOffset(u_Input, coord, ivec2(-1, 0)).rgb;
    value += 0.474 * textureOffset(u_Input, coord, ivec2(0, 0)).rgb;
    value += 0.233 * textureOffset(u_Input, coord, ivec2(1, 0)).rgb;
    value += 0.028 * textureOffset(u_Input, coord, ivec2(2, 0)).rgb;
    value += 0.001 * textureOffset(u_Input, coord, ivec2(3, 0)).rgb;

    imageStore(u_Output, imgCoord, vec4(value, 1.0));
}

#else

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = (vec2(imgCoord) + vec2(0.5)) / vec2(imageSize(u_Output).xy);

    vec3 value = vec3(0.0);
    value += 0.001 * textureOffset(u_Input, coord, ivec2(0, -3)).rgb;
    value += 0.028 * textureOffset(u_Input, coord, ivec2(0, -2)).rgb;
    value += 0.233 * textureOffset(u_Input, coord, ivec2(0, -1)).rgb;
    value += 0.474 * textureOffset(u_Input, coord, ivec2(0, 0)).rgb;
    value += 0.233 * textureOffset(u_Input, coord, ivec2(0, 1)).rgb;
    value += 0.028 * textureOffset(u_Input, coord, ivec2(0, 2)).rgb;
    value += 0.001 * textureOffset(u_Input, coord, ivec2(0, 3)).rgb;

    imageStore(u_Output, imgCoord, vec4(value, 1.0));
}

#endif