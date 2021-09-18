#define PI 3.14159265359

layout(set=0, binding=0) uniform sampler2D u_Depth;
layout(set=0, binding=1) uniform sampler2D u_Normals;

layout(set=0, binding=2, r32f) uniform image2D u_AO;

layout(set=0, binding=3) uniform Globals
{
    vec3 u_ScreenToViewFactors;
    vec3 u_ScreenToViewOffsets;
    vec2 u_InvScreenSize;
    float u_MaxPixelRadiusZ;
};

/* ---------- Debug -------- */

const uint kMaxDebugShapes = 1024;

const uint kDebugSphere = 0;
const uint kDebugRay = 1;

struct DebugShape
{
    vec4 color;
    vec3 srcPos;
    float r;
    vec3 dstPos;
    uint type;
};

layout(set=1, binding=0, std430) buffer DebugShapes
{
    uint d_NumShapes;
    DebugShape d_Shapes[kMaxDebugShapes];
};

layout(set=1, binding=1) uniform DebugGlobals
{
    uvec2 u_DebugCursorPos;
    mat4 u_ViewToWorld;
};

void debugDrawSphere(vec3 pos, float r, vec4 color)
{
    uint entry = atomicAdd(d_NumShapes, 1u);
    if (entry < kMaxDebugShapes)
    {
        DebugShape shape;
        shape.type = kDebugSphere;
        shape.srcPos = pos;
        shape.r = r;
        shape.color = color;

        d_Shapes[entry] = shape;
    }
}

void debugDrawRay(vec3 srcPos, vec3 dstPos, vec4 color)
{
    uint entry = atomicAdd(d_NumShapes, 1u);
    if (entry < kMaxDebugShapes)
    {
        DebugShape shape;
        shape.type = kDebugRay;
        shape.srcPos = srcPos;
        shape.dstPos = dstPos;
        shape.color = color;

        d_Shapes[entry] = shape;
    }
}

/* ------------------------- */


// Get view space position from screen coordinate
vec3 inverseProject(vec2 screenCoord)
{
    float depth = texture(u_Depth, u_InvScreenSize * screenCoord).r;
    float z = u_ScreenToViewFactors.z / (depth - u_ScreenToViewOffsets.z);

    vec2 pos = u_ScreenToViewFactors.xy * (screenCoord - u_ScreenToViewOffsets.xy) * vec2(z);

    return vec3(pos, z);
}

// Screen space - coordinate
// View space - position

void compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = vec2(imgCoord) + vec2(0.5);
    vec3 pos = inverseProject(coord);
    vec3 view = normalize(-pos);

    // Approximate geometric view space normal from differences
    //    vec3 dx = inverseProject(coord + vec2(1.0, 0.0)) - pos;
    //    vec3 dy = inverseProject(coord + vec2(0.0, 1.0)) - pos;
    //    vec3 N = normalize(cross(dy, dx));

    vec3 N = normalize(texture(u_Normals, u_InvScreenSize * coord).xyz);

    const int numSlices = 3;
    const int numSamples = 7;

    float invZ = 1.0 / pos.z;
    float maxPixelRadius = invZ * u_MaxPixelRadiusZ * 1.5;
    maxPixelRadius = min(maxPixelRadius, 300.0);

    float visibility = 0.0;

    float angleOffset = ((gl_GlobalInvocationID.x + gl_GlobalInvocationID.y) % 4u) * ((PI / numSlices)/4.0);

    for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
    {
        float sliceAngle = sliceIndex * PI / numSlices;
        sliceAngle += angleOffset;

        vec3 sliceDir = vec3(cos(sliceAngle), sin(sliceAngle), 0.0);

        // Find max horizon angles in each direction
        float maxCosL = -1.0;
        float maxCosR = -1.0;
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float s = float(sampleIndex+1) / numSamples;
            s *= s;

            vec2 coordOffset = s * maxPixelRadius * sliceDir.xy;

            vec3 samplePosR = inverseProject(coord + coordOffset);
            vec3 samplePosL = inverseProject(coord - coordOffset);

            vec3 horizonR = samplePosR - pos;
            vec3 horizonL = samplePosL - pos;

            float lengthR = length(horizonR);
            float lengthL = length(horizonL);

            float cosR = dot(horizonR/lengthR, view);
            float cosL = dot(horizonL/lengthL, view);

            //            if (gl_GlobalInvocationID.xy == u_DebugCursorPos)
            //            {
            //                vec4 posR = u_ViewToWorld * vec4(samplePosR, 1.0);
            //                vec4 posL = u_ViewToWorld * vec4(samplePosL, 1.0);
            //
            //                debugDrawSphere(posR.xyz, 0.05, vec4(cosR, 0.0, 0.0, 1.0));
            //                debugDrawSphere(posL.xyz, 0.05, vec4(cosR, 0.0, 0.0, 1.0));
            //            }

            cosR = mix(cosR, -1.0, max(lengthR - 0.5, 0.0));
            cosL = mix(cosL, -1.0, max(lengthL - 0.5, 0.0));

            maxCosR = max(maxCosR, clamp(cosR, 0.0, 1.0));
            maxCosL = max(maxCosL, clamp(cosL, 0.0, 1.0));
        }

        // Project normal N onto the slice plane
        vec3 sliceN = cross(sliceDir, view);
        vec3 projN = N - dot(sliceN, N) * sliceN;
        float projNLength = length(projN);

        vec3 sliceOrtho = sliceDir - dot(sliceDir, view) * view;
        float signN = sign(dot(projN, sliceOrtho));

        float cosN = clamp(dot(projN, view) / projNLength, 0.0, 1.0);
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