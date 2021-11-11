#include "Core.shader"

const float kDepthBias = 0.001;
const vec4 kMomentBiasTarget = vec4(0.0, 0.375, 0.0, 0.375);
const float kMomentBiasWeight = 0.0000003;
const float kIntensityScale = 1.02;

// Calculate moment-based shadow intensity from a vector of depth moments and sampled depth
float CalculateMomentShadow(vec4 moments, float depth)
{
    float z = depth * 2.0 - 1.0;
    z -= kDepthBias;

    vec4 b = mix(moments, kMomentBiasTarget, kMomentBiasWeight);

    // Compute entries of the LDL* decomposition of the Hankel moment matrix B
    float L21xD11 = fma(-b.x, b.y, b.z);
    float D11 = fma(-b.x, b.x, b.y);
    float D22a = fma(-b.y, b.y, b.w);
    float D22xD11 = dot(vec2(D22a, -L21xD11), vec2(D11, L21xD11));
    float invD11 = 1.0 / D11;
    float L21 = L21xD11 * invD11;
    float D22 = D22xD11 * invD11;
    float invD22 = 1.0 / D22;

    vec3 c = vec3(1.0, z, z * z);
    // Solve L*c1 = z
    c.y -= b.x;
    c.z -= b.y + L21 * c.y;
    // Scale by inverse D
    c.y *= invD11;
    c.z *= invD22;
    // Solve L^T * c2 = c1 for c2
    c.y -= L21 * c.z;
    c.x -= dot(c.yz, b.xy);

    float invC2 = 1.0 / c.z;
    float p = c.y * invC2;
    float q = c.x * invC2;
    float D =(p * p * 0.25) - q;
    float r = sqrt(D);

    float z1 = -p * 0.5 - r;
    float z2 = -p * 0.5 + r;

    vec4 mask = z2 < z ? vec4(z1, z, 1.0, 1.0) :
    (z1 < z ? vec4(z, z1, 0.0, 1.0) : vec4(0.0, 0.0, 0.0, 0.0));

    float quotient = (mask.x*z2 - b.x*(mask.x + z2) +b.y) / ((z2 - mask.y)*(z -z1));
    float intensity = mask.z + mask.w * quotient;
    intensity *= kIntensityScale;

    return 1.0 - clamp(intensity, 0.0, 1.0);
}
