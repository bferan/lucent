#include "Core.shader"

layout(set=0, binding=0) uniform sampler2D u_Input;
layout(set=0, binding=1, r32f) uniform image2D u_Output;

layout(local_size_x=8, local_size_y=8) in;

layout(set=0, binding=2) uniform Parameters
{
    ivec2 u_Offset;
};

// Perform min filtering on input texture
void Compute()
{
    ivec2 dstCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 srcCoord = dstCoord * ivec2(2);

    // Offset to sample positions enables better filtering of NPOT textures
    vec4 depths;
    depths.x = texelFetch(u_Input, srcCoord, 0).r;
    depths.y = texelFetch(u_Input, srcCoord + ivec2(u_Offset.x, 0), 0).r;
    depths.z = texelFetch(u_Input, srcCoord + ivec2(0, u_Offset.y), 0).r;
    depths.w = texelFetch(u_Input, srcCoord + u_Offset, 0).r;

    float z = min(min(depths.x, depths.y), min(depths.z, depths.w));

    imageStore(u_Output, dstCoord, vec4(z, vec3(0.0)));
}