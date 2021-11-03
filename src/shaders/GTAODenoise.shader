
layout(local_size_x=8, local_size_y=8) in;

layout(set=0, binding=0) uniform sampler2D u_AORaw;
layout(set=0, binding=1, r32f) uniform image2D u_AODenoised;

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 invSize = 1.0 / vec2(imageSize(u_AODenoised).xy);

    vec2 coord0 = vec2(imgCoord) * invSize;
    vec2 coord1 = vec2(imgCoord + ivec2(0, 2)) * invSize;
    vec2 coord2 = vec2(imgCoord + ivec2(2, 0)) * invSize;
    vec2 coord3 = vec2(imgCoord + ivec2(2, 2)) * invSize;

    vec4 v0 = textureGather(u_AORaw, coord0, 0);
    vec4 v1 = textureGather(u_AORaw, coord1, 0);
    vec4 v2 = textureGather(u_AORaw, coord2, 0);
    vec4 v3 = textureGather(u_AORaw, coord3, 0);

    float value = 0.0;

    value += v0.x;
    value += v0.y;
    value += v0.z;
    value += v0.w;

    value += v1.x;
    value += v1.y;
    value += v1.z;
    value += v1.w;

    value += v2.x;
    value += v2.y;
    value += v2.z;
    value += v2.w;

    value += v3.x;
    value += v3.y;
    value += v3.z;
    value += v3.w;

    value /= 16.0;

    //value = v0.x;

    imageStore(u_AODenoised, imgCoord, vec4(value, 0.0, 0.0, 1.0));
}