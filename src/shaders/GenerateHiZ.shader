layout(set=0, binding=0) uniform sampler2D u_Input;
layout(set=0, binding=1, r32f) uniform image2D u_Output;

layout(local_size_x=8, local_size_y=8) in;

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = (vec2(imgCoord) + vec2(0.5)) / imageSize(u_Output);

    vec4 depths = textureGather(u_Input, coord);
    float z = min(min(depths.x, depths.y), min(depths.z, depths.w));

    imageStore(u_Output, imgCoord, vec4(z, vec3(0.0)));
}