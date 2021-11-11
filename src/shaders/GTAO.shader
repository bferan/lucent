#include "Core.shader"
#include "View.shader"

layout(set=1, binding=0) uniform sampler2D u_Depth;
layout(set=1, binding=1) uniform sampler2D u_Normals;
layout(set=1, binding=2, r32f) uniform image2D u_AO;
layout(set=1, binding=3) uniform Globals
{
    float u_ViewToScreenZ;
};

// Noise patterns as outlined in:
// "Practical Realtime Strategies for Accurate Indirect Occlusion"
float DirectionSpatialNoise(int x, int y)
{
    return (1.0/16.0) * ((((x + y) & 0x3) << 2) + (x & 0x3));
}

float OffsetSpatialNoise(int x, int y)
{
    return (1.0/4.0) * ((y - x) & 0x3);
}

float GetDepth(vec2 coord)
{
    return textureLod(u_Depth, coord, 0.0).r;
}

layout(local_size_x=8, local_size_y=8) in;

const int kNumSlices = 3;
const int kNumSamples = 7;

float GTAOContribution(float angle, float angleN, float cosN, float sinN)
{
    return 0.25 * (-cos(2.0 * angle - angleN) + cosN + 2.0 * angle * sinN);
}

// Compute Ground-truth Ambient Occlusion vibility at given screen coordinates
float GTAOVisibility(vec2 coord, ivec2 pixelCoord)
{
    vec3 pos = ScreenToView(coord, GetDepth(coord));
    vec3 V = normalize(-pos);
    vec3 N = normalize(2.0 * texture(u_Normals, coord).xyz - 1.0);

    const float sampleStep = 1.0 / kNumSamples;

    float invZ = 1.0 / pos.z;
    float maxRadius = invZ * u_ViewToScreenZ;
    maxRadius = clamp(maxRadius, 0.01, 0.25);

    float angleOffset = DirectionSpatialNoise(pixelCoord.x, pixelCoord.y) * PI;
    float sampleOffset = OffsetSpatialNoise(pixelCoord.x, pixelCoord.y);

    float visibility = 0.0;
    for (int sliceIndex = 0; sliceIndex < kNumSlices; ++sliceIndex)
    {
        float sliceAngle = sliceIndex * PI / kNumSlices;
        sliceAngle += angleOffset;

        vec3 sliceDir = vec3(cos(sliceAngle) / u_AspectRatio, sin(sliceAngle), 0.0);

        float maxCosL = -1.0;
        float maxCosR = -1.0;

        // Find max horizon angles in each direction
        float s = sampleOffset * sampleStep + sampleStep;
        for (int sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex)
        {
            vec2 coordOffset = s * maxRadius * sliceDir.xy;

            vec2 coordR = coord + coordOffset;
            vec2 coordL = coord - coordOffset;

            vec3 samplePosR = ScreenToView(coordR, GetDepth(coordR));
            vec3 samplePosL = ScreenToView(coordL, GetDepth(coordL));

            vec3 horizonR = samplePosR - pos;
            vec3 horizonL = samplePosL - pos;

            float lengthR = length(horizonR);
            float lengthL = length(horizonL);

            float cosR = dot(horizonR/lengthR, V);
            float cosL = dot(horizonL/lengthL, V);

            // Lerp cosine of angle toward -1 with distance to give less weight to far occlusion
            cosR = mix(cosR, -1.0, max(lengthR - 0.5, 0.0));
            cosL = mix(cosL, -1.0, max(lengthL - 0.5, 0.0));

            maxCosR = max(maxCosR, cosR);
            maxCosL = max(maxCosL, cosL);

            s += sampleStep;
        }

        // Project normal N onto the slice plane
        vec3 sliceN = cross(sliceDir, V);
        vec3 projN = N - dot(sliceN, N) * sliceN;
        float projNLength = length(projN);

        vec3 sliceOrtho = sliceDir - dot(sliceDir, V) * V;
        float signN = sign(dot(projN, sliceOrtho));

        float cosN = clamp(dot(projN, V) / projNLength, 0.0, 1.0);
        float angleN = signN * acos(cosN);

        float angleR = acos(maxCosR);
        float angleL = -acos(maxCosL);

        // Clamp horizon angles to hemisphere around normal N
        angleR = angleN + min(angleR - angleN, 0.5 * PI);
        angleL = angleN + max(angleL - angleN, -0.5 * PI);

        float sinN = sin(angleN);
        float a = GTAOContribution(angleL, angleN, cosN, sinN) +
            GTAOContribution(angleR, angleN, cosN, sinN);

        visibility += projNLength * a;
    }
    return visibility / kNumSlices;
}

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = vec2(imgCoord) + vec2(0.5);
    vec2 imgSize = vec2(imageSize(u_AO).xy);
    coord /= imgSize;

    float visibility = GTAOVisibility(coord, imgCoord);

    imageStore(u_AO, imgCoord, vec4(visibility, 0.0, 0.0, 1.0));
}