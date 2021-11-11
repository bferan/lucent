layout(set=0, binding=0) uniform sampler2D u_Input;
layout(set=0, binding=1) uniform sampler2D u_Bloom;
layout(set=0, binding=2, rgba8) uniform image2D u_Output;

layout(set=0, binding=3) uniform Parameters
{
    float u_BloomStrength;

    float u_VignetteIntensity;
    float u_VignetteExtent;
};

vec3 ToneMap(vec3 x)
{
    // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec3 Vignette(vec3 value, vec2 uv)
{
    // https://www.shadertoy.com/view/lsKSWR
    uv *=  1.0 - uv.yx;
    float mask = uv.x * uv.y * u_VignetteIntensity;
    mask = pow(mask, u_VignetteExtent);
    return mask * value;
}

layout(local_size_x=8, local_size_y=8) in;

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = (vec2(imgCoord) + vec2(0.5)) / imageSize(u_Output).xy;
    vec3 value = texture(u_Input, coord).rgb;

    #ifdef PP_BLOOM
    vec3 bloom = texture(u_Bloom, coord).rgb;
    value = mix(value, bloom, u_BloomStrength);
    #endif

    #ifdef PP_TONEMAP
    value = ToneMap(value);
    #endif

    #ifdef PP_VIGNETTE
    value = Vignette(value, coord);
    #endif

    imageStore(u_Output, imgCoord, vec4(value, 1.0));
}