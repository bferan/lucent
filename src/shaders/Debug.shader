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

// Draw a sphere at world-space coordinates
void DebugDrawSphere(vec3 pos, float r, vec4 color)
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

// Draw a ray between world space positions
void DebugDrawRay(vec3 srcPos, vec3 dstPos, vec4 color)
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