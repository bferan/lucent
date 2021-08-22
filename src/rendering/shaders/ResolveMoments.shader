#include "shared/VertexLayout"

layout(location=0) out vec4 o_Moments;

uniform layout(set=0, binding=0) sampler2DMS u_Depth;

void vert()
{
    gl_Position = vec4(a_Position, 1.0);
}

vec4 calculateMoments(float depth)
{
    float d = depth * 2.0 - 1.0;
    float d2 = d * d;
    return vec4(d, d2, d2 * d, d2 * d2);
}

const uint sampleCount = 8;

void frag()
{
    vec4 moments = vec4(0);
    ivec2 coord = ivec2(gl_FragCoord.xy);

    const float weight = 1.0/float(sampleCount);
    for (int i = 0; i < sampleCount; ++i)
    {
        moments += weight * calculateMoments(texelFetch(u_Depth, coord, i).r);
    }

    o_Moments = moments;
}