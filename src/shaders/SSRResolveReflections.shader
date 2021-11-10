#include "Core.shader"
#include "View.shader"

layout(set=1, binding=0) uniform sampler2D u_Rays;
layout(set=1, binding=1) uniform sampler2D u_ConvolvedScene;
layout(set=1, binding=2) uniform sampler2D u_Depth;
layout(set=1, binding=3) uniform sampler2D u_MetalRoughness;

layout(set=1, binding=4, rgba32f) uniform image2D u_Result;

layout(local_size_x=8, local_size_y=8) in;

const int kIterations = 5;

float AlphaToConeAngle(float alpha)
{
    alpha = 1.0;
    const float eta = 0.244;
    return acos(pow(eta, 1.0 / (alpha + 1.0)));
}

float ConeRadius(float length, float base)
{
    return base * (sqrt(base * base + 4.0 * (length * length)) - base) / (4.0 * length);
}

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = vec2(imgCoord) + vec2(0.5);
    vec2 imgSize = vec2(imageSize(u_Result).xy);
    vec2 sceneSize = vec2(textureSize(u_ConvolvedScene, 0).xy);
    coord /= imgSize;

    vec4 rayEnd = texture(u_Rays, coord);

    float rough = texture(u_MetalRoughness, coord).g;
    float angle = AlphaToConeAngle(rough * rough);
    float lengthToBaseFactor = 2.0 * tan(angle);

    float maxDim = max(sceneSize.x, sceneSize.y);

    vec2 rayUnit = rayEnd.xy - coord;
    float rayLength = length(rayUnit);
    rayUnit /= rayLength;

//    float weight = 0.5;
//    for (int i = 0; i < kIterations; ++i)
//    {
//        float baseLength = lengthToBaseFactor * rayLength;
//        float radius = ConeRadius(rayLength, baseLength);
//
//        vec2 sampleCoord = coord + (rayLength - radius) * rayUnit;
//
//        float mip = log2(radius * maxDim);
//
//        result += weight * textureLod(u_ConvolvedScene, sampleCoord, mip).rgb;
//
//        rayLength -= (2.0 * radius);
//        weight *= 0.5;
    //    }

    vec4 result = vec4(textureLod(u_ConvolvedScene, rayEnd.xy, 2.0).rgb, 1.0);
    result = clamp(result, vec4(0.0), vec4(1.0));

    if (rayEnd.x <= 0.0 || rayEnd.y <= 0.0)
    result = vec4(0.0);

    imageStore(u_Result, imgCoord, result);
}