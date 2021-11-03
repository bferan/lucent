#define PI 3.14159265358979323846

layout(set=0, binding=0) uniform sampler2D u_MinZ;
layout(set=0, binding=2) uniform sampler2D u_HaltonNoise;
layout(set=0, binding=3) uniform sampler2D u_Normals;
layout(set=0, binding=4) uniform sampler2D u_MetalRoughness;

layout(set=0, binding=5, rgba32f) uniform image2D u_Result;

layout(set=0, binding=6) uniform Globals
{
    vec4 u_ScreenToView;
    mat4 u_Proj;
};

vec3 screenToView(vec2 coord, out float depth)
{
    depth = textureLod(u_MinZ, coord, 0.0).r;
    float z = u_ScreenToView.w / (depth - u_ScreenToView.z);
    vec2 pos = u_ScreenToView.xy * (coord - vec2(0.5)) * vec2(z);

    return vec3(pos, z);
}

vec3 intersectBounds(vec3 pos, vec3 dir, int level, vec2 cellOffset, vec2 cellBias, out float distance)
{
    vec2 numCells = vec2(textureSize(u_MinZ, level));
    vec2 cell = floor(pos.xy * numCells) + cellOffset;
    cell /= numCells;

    vec2 boundaryDist = (cell - pos.xy) / dir.xy;
    distance = min(boundaryDist.x, boundaryDist.y);

    pos += distance * dir;
    pos.xy += (boundaryDist.x < boundaryDist.y) ? vec2(cellBias.x, 0.0) : vec2(0.0, cellBias.y);

    return pos;
}

vec3 snapToCell(vec3 pos, int level)
{
    vec2 numCells = vec2(textureSize(u_MinZ, level));
    vec2 cell = floor(pos.xy * numCells) + vec2(0.5);
    cell /= numCells;
    return vec3(cell.xy, pos.z);
}

vec3 sampleGGX(vec3 V, float alpha, vec2 rand)
{
    // Sampling the GGX Distribution of Visible Normals [Heitz2018]
    // http://jcgt.org/published/0007/04/01/

    vec3 Vh = normalize(vec3(V.xy * alpha, V.z));

    // Construct axes of disk orthogonal to view direction
    float len2 = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = len2 > 0.0 ? inversesqrt(len2) * vec3(-Vh.y, Vh.x, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T2 = cross(Vh, T1);

    // Sample random point on disk
    float radius = sqrt(rand.x);
    float phi = 2.0 * PI * rand.y;
    float t1 = radius * cos(phi);
    float t2 = radius * sin(phi);

    // Compress vertical coordinate proportional to projection of lower half disk
    float rescale = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - rescale) * sqrt(1.0 - t1 * t1) + rescale * t2;

    // Reproject back onto hemisphere
    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(1.0 - t1 * t1 - t2 * t2, 0.0)) * Vh;

    vec3 N = normalize(vec3(Nh.xy * alpha, max(Nh.z, 0.0)));
    return N;
}

float GGX_D(float NdotM, float a2)
{
    float denom = NdotM * NdotM * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GGX_SmithG1(float NdotV, float a2)
{
    float denom = NdotV + sqrt(a2 + (1.0 - a2) * NdotV * NdotV);
    return 2.0 * NdotV / denom;
}

float GGX_SmithCorrelatedG2(float NdotL, float NdotV, float a2)
{
    float NdotV2 = NdotV * NdotV;
    float NdotL2 = NdotL * NdotL;

    float lambdaV = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotV2))/NdotV2)) * 0.5;
    float lambdaL = (-1.0 + sqrt(1.0 + (a2 * (1.0 - NdotL2))/NdotL2)) * 0.5;
    return 1.0 / (1.0 + lambdaV + lambdaL);
}

const float kCrossingBias = 0.00001;
const int kStopLevel = 0;
const int kMaxLevel = 10;
const int kMaxIterations = 128;
const float kBias = 0.0;

void Compute()
{
    ivec2 imgCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 coord = vec2(imgCoord) + vec2(0.5);
    vec2 imgSize = vec2(imageSize(u_Result).xy);
    coord /= imgSize;

    // Compute view space ray configuration
    float depth;
    vec3 pos = screenToView(coord, depth);
    vec3 V = normalize(-pos);

    vec3 N = normalize(2.0 * texture(u_Normals, coord).xyz - 1.0);

    vec3 T = N.z < 0.999 ? cross(N, vec3(0.0, 0.0, 1.0)) : vec3(1.0, 0.0, 0.0);
    vec3 B = cross(T, N);

    vec2 noiseCoord = (imgSize / vec2(textureSize(u_HaltonNoise, 0).xy)) * coord;
    vec2 rand = texture(u_HaltonNoise, noiseCoord).xy;
    rand.x = mix(rand.x, 1.0, kBias);

    float rough = texture(u_MetalRoughness, coord).g;
    float alpha = rough * rough;
    float alpha2 = alpha * alpha;

    vec3 Vm = inverse(mat3(B, T, N)) * V;
    vec3 Nm = sampleGGX(Vm, alpha, rand);
    vec3 Rm = (2.0 * dot(Vm, Nm) * Nm) - Vm;

    vec3 R = mat3(B, T, N) * Rm;

    float pdf = GGX_SmithG1(Vm.z, alpha2) * GGX_D(Nm.z, alpha2) / (4.0 * Vm.z);

    vec3 nextPos = pos + R;
    vec4 nextClip = u_Proj * vec4(nextPos, 1.0);
    vec3 nextCoord = nextClip.xyz / nextClip.w;
    nextCoord.xy = vec2(0.5) * nextCoord.xy + vec2(0.5);

    vec3 screenPos = vec3(coord, depth);
    vec3 screenDir = nextCoord - screenPos;

    vec2 cellOffset = vec2((screenDir.x > 0.0) ? 1.0 : 0.0, (screenDir.y > 0.0) ? 1.0 : 0.0);
    vec2 cellBias = kCrossingBias * vec2(vec2(-1.0) + 2.0 * cellOffset);

    // Trace ray through the depth pyramid
    int level = 0;
    int iterations = 0;
    float boundaryDist = 0.0;
    float depthDist = 0.0;

    screenPos = snapToCell(screenPos, level);
    screenPos = intersectBounds(screenPos, screenDir, level, cellOffset, cellBias, boundaryDist);

    while (level >= kStopLevel && iterations < kMaxIterations)
    {
        // Find planes
        vec3 boundaryPos = intersectBounds(screenPos, screenDir, level, cellOffset, cellBias, boundaryDist);

        float z = textureLod(u_MinZ, screenPos.xy, level).r;
        depthDist = (z - screenPos.z) / screenDir.z;

        if (screenDir.z < 0)
        {
            if (screenPos.z < z)
            {
                screenPos = boundaryPos;
                level = min(level + 2, kMaxLevel);
            } else
            {
                depthDist = 0.0;
            }
        }
        else if (boundaryDist < depthDist)
        {
            // Advance to boundary
            screenPos = boundaryPos;
            level = min(level + 2, kMaxLevel);
        }

        level--;
        iterations++;
    }

    screenPos += depthDist * screenDir;

    float attenuation = 1.0;
    vec4 result = vec4(screenPos.xy, pdf, attenuation);

    if (screenDir.z < 0.0)
    result = vec4(0.0);

    if (iterations >= kMaxIterations)
    result = vec4(0.0);

    imageStore(u_Result, imgCoord, result);
}