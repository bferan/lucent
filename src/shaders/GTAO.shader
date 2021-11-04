//#include "Debug"

#define PI 3.14159265359

layout(local_size_x=8, local_size_y=8) in;

layout(set=0, binding=0) uniform sampler2D u_Depth;
layout(set=0, binding=1) uniform sampler2D u_Normals;
layout(set=0, binding=2, r32f) uniform image2D u_AO;
layout(set=0, binding=3) uniform Globals
{
    vec4 u_ScreenToView;
    float u_AspectRatio;
    float u_ViewToScreenZ;
};

// Get view space position from screen coordinate
vec3 ScreenToView(vec2 coord)
{
    float depth = textureLod(u_Depth, coord, 0.0).r;
    float z = u_ScreenToView.w / (depth - u_ScreenToView.z);
    vec2 pos = u_ScreenToView.xy * (coord - vec2(0.5)) * vec2(z);

    return vec3(pos, z);
}

float DirectionSpatialNoise(int x, int y)
{
    return (1.0/16.0) * ((((x + y) & 0x3) << 2) + (x & 0x3));
}

float OffsetSpatialNoise(int x, int y)
{
    return (1.0/4.0) * ((y - x) & 0x3);
}

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = vec2(imgCoord) + vec2(0.5);
    vec2 imgSize = vec2(imageSize(u_AO).xy);
    coord /= imgSize;

    vec3 pos = ScreenToView(coord);
    vec3 V = normalize(-pos);

    vec3 N = normalize(2.0 * texture(u_Normals, coord).xyz - 1.0);

//        if (gl_GlobalInvocationID.xy == u_DebugCursorPos)
//        {
//            DebugDrawSphere((u_ViewToWorld * vec4(pos, 1.0)).xyz, 0.05, vec4(0.0, 1.0, 0.0, 1.0));
//            DebugDrawSphere((u_ViewToWorld * vec4(pos + N, 1.0)).xyz, 0.05, vec4(0.0, 1.0, 0.0, 1.0));
//            DebugDrawSphere((u_ViewToWorld * vec4(pos + 2.0 * N, 1.0)).xyz, 0.05, vec4(0.0, 1.0, 0.0, 1.0));
//        }

    const int numSlices = 3;
    const int numSamples = 7;
    const float sampleStep = 1.0 / numSamples;

    float invZ = 1.0 / pos.z;
    float maxRadius = invZ * u_ViewToScreenZ * 1.0;
    maxRadius = clamp(maxRadius, 0.01, 0.25);

    float angleOffset = DirectionSpatialNoise(imgCoord.x, imgCoord.y) * PI;
    float sampleOffset = OffsetSpatialNoise(imgCoord.x, imgCoord.y);

    float visibility = 0.0;
    for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
    {
        float sliceAngle = sliceIndex * PI / numSlices;
        sliceAngle += angleOffset;

        vec3 sliceDir = vec3(cos(sliceAngle) / u_AspectRatio, sin(sliceAngle), 0.0);

        // Find max horizon angles in each direction
        float maxCosL = -1.0;
        float maxCosR = -1.0;

        float s = sampleOffset * sampleStep + sampleStep;
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            vec2 coordOffset = s * maxRadius * sliceDir.xy;

            vec3 samplePosR = ScreenToView(coord + coordOffset);
            vec3 samplePosL = ScreenToView(coord - coordOffset);

            vec3 horizonR = samplePosR - pos;
            vec3 horizonL = samplePosL - pos;

            float lengthR = length(horizonR);
            float lengthL = length(horizonL);

            float cosR = dot(horizonR/lengthR, V);
            float cosL = dot(horizonL/lengthL, V);

//                        if (gl_GlobalInvocationID.xy == u_DebugCursorPos)
//                        {
//                            vec4 posR = u_ViewToWorld * vec4(samplePosR, 1.0);
//                            vec4 posL = u_ViewToWorld * vec4(samplePosL, 1.0);
//
//                            DebugDrawSphere(posR.xyz, 0.05, vec4(lengthR, 0.0, 0.0, 1.0));
//                            DebugDrawSphere(posL.xyz, 0.05, vec4(lengthL, 0.0, 0.0, 1.0));
//                        }

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
        angleR = angleN + min(angleR - angleN, 0.5*PI);
        angleL = angleN + max(angleL - angleN, -0.5*PI);

        float sinN = sin(angleN);
        float a =
        0.25 * (-cos(2.0 * angleL - angleN) + cosN + 2.0 * angleL * sinN) +
        0.25 * (-cos(2.0 * angleR - angleN) + cosN + 2.0 * angleR * sinN);

        visibility += projNLength * a;
    }
    visibility /= numSlices;

    imageStore(u_AO, imgCoord, vec4(visibility, 0.0, 0.0, 1.0));
}