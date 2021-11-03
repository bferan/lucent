layout(set=0, binding=0) uniform sampler2D u_Input;
layout(set=0, binding=1, rgba8) uniform image2D u_Output;

layout(local_size_x=8, local_size_y=8) in;

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
    float vig = uv.x * uv.y * 15.0; // multiply with sth for intensity
    vig = pow(vig, 0.25); // change pow for modifying the extend of the  vignette
    return vig * value;
}

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = (vec2(imgCoord) + vec2(0.5)) / imageSize(u_Output).xy;

    vec3 value = texture(u_Input, coord).rgb;
    //value = ToneMap(value);
    value = Vignette(value, coord);

    imageStore(u_Output, imgCoord, vec4(value, 1.0));
}