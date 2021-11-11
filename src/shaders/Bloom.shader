// Averaging function used in downsample
#ifndef AVERAGE
#define AVERAGE Average
#endif

layout(set=0, binding=0) uniform sampler2D u_Input;
layout(set=0, binding=1, rgba32f) uniform image2D u_Output;

#ifdef BLOOM_UPSAMPLE
layout(set=0, binding=2) uniform sampler2D u_InputHigh;
layout(set=0, binding=3) uniform Parameters
{
    float u_FilterRadius;
    float u_Strength;
};
#endif

// Regular arithmetic mean
vec3 Average(vec3 p0, vec3 p1, vec3 p2, vec3 p3)
{
    return 0.25 * (p0 + p1 + p2 + p3);
}

float Luminance(vec3 v)
{
    return 0.2126 * v.r + 0.7152 * v.g + 0.0722 * v.b;
}

float KarisWeight(vec3 v)
{
    return 1.0 / (1.0 + Luminance(v));
}

// Inverse-luminance weighted mean of 4 samples
vec3 KarisAverage(vec3 p0, vec3 p1, vec3 p2, vec3 p3)
{
    vec4 weights = vec4(KarisWeight(p0), KarisWeight(p1), KarisWeight(p2), KarisWeight(p3));

    vec3 result = vec3(0.0);
    result += weights.x * p0;
    result += weights.y * p1;
    result += weights.z * p2;
    result += weights.w * p3;

    return result / dot(weights, 1.0.rrrr);
}


// Perform a 13-tap filter on the input image as described in
// "Next Generation Post Processing in Call of Duty: Advanced Warfare"
vec3 Downsample13Tap(vec2 coord, vec2 texelSize)
{
    vec3 p0  = texture(u_Input, coord + texelSize * vec2(-1.0, -1.0)).rgb;
    vec3 p1  = texture(u_Input, coord + texelSize * vec2(0.0, -1.0)).rgb;
    vec3 p2  = texture(u_Input, coord + texelSize * vec2(1.0, -1.0)).rgb;

    vec3 p3  = texture(u_Input, coord + texelSize * vec2(-0.5, -0.5)).rgb;
    vec3 p4  = texture(u_Input, coord + texelSize * vec2(0.5, -0.5)).rgb;

    vec3 p5  = texture(u_Input, coord + texelSize * vec2(-1.0, 0.0)).rgb;
    vec3 p6  = texture(u_Input, coord).rgb;
    vec3 p7  = texture(u_Input, coord + texelSize * vec2(1.0, 0.0)).rgb;

    vec3 p8  = texture(u_Input, coord + texelSize * vec2(-0.5, 0.5)).rgb;
    vec3 p9  = texture(u_Input, coord + texelSize * vec2(0.5, 0.5)).rgb;

    vec3 p10 = texture(u_Input, coord + texelSize * vec2(-1.0, 1.0)).rgb;
    vec3 p11 = texture(u_Input, coord + texelSize * vec2(0.0, 1.0)).rgb;
    vec3 p12 = texture(u_Input, coord + texelSize * vec2(1.0, 1.0)).rgb;

    vec3 average = vec3(0.0);
    average += 0.5   * AVERAGE(p3, p4, p8, p9);
    average += 0.125 * AVERAGE(p0, p1, p5, p6);
    average += 0.125 * AVERAGE(p1, p2, p6, p7);
    average += 0.125 * AVERAGE(p6, p7, p11, p12);
    average += 0.125 * AVERAGE(p5, p6, p10, p11);
    return average;
}

// Upsample the input image with a tent filter as described in the presentation above
vec3 UpsampleTentFilter(vec2 coord, vec2 texelSize)
{
    const float kWeight = 1.0 / 16.0;
    const float kTexelRadius = 4.0;

    vec2 radius = texelSize * kTexelRadius;
    vec3 result = vec3(0.0);

    result +=       texture(u_Input, coord + radius * vec2(-1.0, -1.0)).rgb;
    result += 2.0 * texture(u_Input, coord + radius * vec2(0.0, -1.0)).rgb;
    result +=       texture(u_Input, coord + radius * vec2(1.0, -1.0)).rgb;

    result += 2.0 * texture(u_Input, coord + radius * vec2(-1.0, 0.0)).rgb;
    result += 4.0 * texture(u_Input, coord).rgb;
    result += 2.0 * texture(u_Input, coord + radius * vec2(1.0, 0.0)).rgb;

    result +=       texture(u_Input, coord + radius * vec2(-1.0, 1.0)).rgb;
    result += 2.0 * texture(u_Input, coord + radius * vec2(0.0, 1.0)).rgb;
    result +=       texture(u_Input, coord + radius * vec2(1.0, 1.0)).rgb;

    return result * kWeight;
}

layout(local_size_x=8, local_size_y=8) in;

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 texelSize = 1.0 / vec2(imageSize(u_Output).xy);
    vec2 coord = (vec2(imgCoord) + vec2(0.5)) * texelSize;

    #ifdef BLOOM_UPSAMPLE
    vec3 blurred = UpsampleTentFilter(coord, texelSize);
    vec3 result = texture(u_InputHigh, coord).rgb;
    result = mix(result, blurred, u_Strength);

    #else
    vec3 result = Downsample13Tap(coord, texelSize);
    #endif

    imageStore(u_Output, imgCoord, vec4(result, 1.0));
}
