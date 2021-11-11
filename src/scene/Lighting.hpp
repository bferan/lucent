#pragma once

namespace lucent
{

//! Directional light component
struct DirectionalLight
{
    Color color;

    // TODO: Extract these as render settings
    static constexpr int kNumCascades = 4;
    static constexpr int kMapWidth = 2048;

    struct Cascade
    {
        float start;
        float end;

        float worldSpaceTexelSize;

        Vector3 position;
        float width;
        float depth;
        Matrix4 projection;

        Vector3 offset;
        Vector3 scale;
        Vector4 frontPlane;
    };

    Cascade cascades[kNumCascades];
};

struct Environment
{
    Texture* cubeMap;
    Texture* irradianceMap;
    Texture* specularMap;
    Texture* BRDF;
};

}