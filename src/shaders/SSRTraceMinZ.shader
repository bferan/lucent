#include "Core.shader"
#include "View.shader"

layout(set=1, binding=0) uniform sampler2D u_MinZ;
layout(set=1, binding=1) uniform sampler2D u_Normals;
layout(set=1, binding=3, rgba32f) uniform image2D u_Result;

const float kCrossingBias = 0.00001;
const int kStopLevel = 0;
const int kMaxLevel = 10;
const int kMaxIterations = 128;

const float kLinearDepthThreshold = 0.1;
const vec3 kScreenLimit = vec3(0.9999);

vec3 IntersectBounds(vec3 pos, vec3 dir, int level, vec2 cellOffset, vec2 cellBias, out float distance)
{
    vec2 numCells = vec2(textureSize(u_MinZ, level));
    vec2 cell = floor(pos.xy * numCells) + cellOffset;
    cell /= numCells;

    vec2 boundaryDist = (cell - pos.xy) / dir.xy;
    distance = min(boundaryDist.x, boundaryDist.y);

    // Advance ray pos into next cell, and add a small bias to ensure it's not placed on the border
    pos += distance * dir;
    pos.xy += (boundaryDist.x < boundaryDist.y) ? vec2(cellBias.x, 0.0) : vec2(0.0, cellBias.y);

    return pos;
}

vec3 SnapToCell(vec3 pos, int level)
{
    vec2 numCells = vec2(textureSize(u_MinZ, level));
    vec2 cell = floor(pos.xy * numCells) + vec2(0.5);
    cell /= numCells;
    return vec3(cell.xy, pos.z);
}

vec2 TraceRay(vec3 screenPos, vec3 screenDir, vec2 cellOffset, vec2 cellBias)
{
    // Trace ray through the depth pyramid
    int level = 0;
    int iterations = 0;
    float boundaryDist = 0.0;
    float depthDist = 0.0;
    float depthPlane = 0.0;
    float startDepth = screenPos.z;

    screenPos = SnapToCell(screenPos, level);
    screenPos = IntersectBounds(screenPos, screenDir, level, cellOffset, cellBias, boundaryDist);

    while (level >= kStopLevel && iterations < kMaxIterations)
    {
        vec3 boundaryPos = IntersectBounds(screenPos, screenDir, level, cellOffset, cellBias, boundaryDist);

        depthPlane = textureLod(u_MinZ, screenPos.xy, level).r;
        depthDist = (depthPlane - screenPos.z) / screenDir.z;

        if (screenPos.z < depthPlane && (boundaryDist < depthDist || screenDir.z < 0))
        {
            screenPos = boundaryPos;
            level = min(level + 2, kMaxLevel);
        }
        level--;
        iterations++;
    }
    float linearDiff = GetLinearDepth(screenPos.z) - GetLinearDepth(depthPlane);

    vec2 result = screenPos.xy;
    if (startDepth == 1.0 || any(greaterThanEqual(abs(screenPos), kScreenLimit)) || linearDiff > kLinearDepthThreshold)
    {
        result = vec2(0.0);
    }
    return result;
}

layout(local_size_x=8, local_size_y=8) in;

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = vec2(imgCoord) + vec2(0.5);
    vec2 imgSize = vec2(imageSize(u_Result).xy);
    coord /= imgSize;

    // Compute view space ray configuration
    float nonlinearDepth = textureLod(u_MinZ, coord, 0).r;
    vec3 pos = ScreenToView(coord, nonlinearDepth);
    vec3 V = normalize(-pos);
    vec3 N = normalize(2.0 * texture(u_Normals, coord).xyz - 1.0);
    vec3 R = (2.0 * dot(V, N) * N) - V;

    vec3 nextPos = pos + R;
    vec4 nextClip = u_ViewToScreen * vec4(nextPos, 1.0);
    vec3 nextCoord = nextClip.xyz / nextClip.w;
    nextCoord.xy = vec2(0.5) * nextCoord.xy + vec2(0.5);

    vec3 screenPos = vec3(coord, nonlinearDepth);
    vec3 screenDir = nextCoord - screenPos;

    vec2 cellOffset = vec2((screenDir.x > 0.0) ? 1.0 : 0.0, (screenDir.y > 0.0) ? 1.0 : 0.0);
    vec2 cellBias = kCrossingBias * vec2(vec2(-1.0) + 2.0 * cellOffset);

    vec2 result = TraceRay(screenPos, screenDir, cellOffset, cellBias);

    imageStore(u_Result, imgCoord, vec4(result, 0.0, 1.0));
}