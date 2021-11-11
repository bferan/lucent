#include "VertexInput.shader"

layout(location=0) out vec4 o_Moments;

uniform layout(set=0, binding=0) sampler2DMS u_Depth;

const uint kSampleCount = 8;
const float kWeight = 1.0 / float(kSampleCount);

vec4 CalculateMoments(float depth)
{
    float d = depth * 2.0 - 1.0;
    float d2 = d * d;
    return vec4(d, d2, d2 * d, d2 * d2);
}

void Vertex()
{
    gl_Position = vec4(a_Position, 1.0);
}

// Resolves a vector of depth moments from a multisample depth buffer
void Fragment()
{
    vec4 moments = vec4(0);
    ivec2 coord = ivec2(gl_FragCoord.xy);

    for (int i = 0; i < kSampleCount; ++i)
    {
        moments += kWeight * CalculateMoments(texelFetch(u_Depth, coord, i).r);
    }
    o_Moments = moments;
}