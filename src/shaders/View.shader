
// Shared view parameters
layout(set=0, binding=0) uniform View
{
    vec4 u_ScreenToView;
    mat4 u_ViewToWorld;
    mat4 u_WorldToView;
    mat4 u_ViewToScreen;
    float u_AspectRatio;
};

float GetLinearDepth(float nonlinearDepth)
{
    return u_ScreenToView.w / (nonlinearDepth - u_ScreenToView.z);
}

// Get view space position from screen coordinate in range [0,1]^2
vec3 ScreenToView(vec2 coord, float nonlinearDepth)
{
    float z = GetLinearDepth(nonlinearDepth);
    vec2 pos = u_ScreenToView.xy * (coord - vec2(0.5)) * vec2(z);

    return vec3(pos, z);
}



