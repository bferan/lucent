#pragma once

namespace lucent
{

struct DirectionalLight
{
    Color color;

    static constexpr int kNumCascades = 4;
    static constexpr int kMapWidth = 2048;

    struct Cascade
    {
        float start;
        float end;

        float worldSpaceTexelSize;

        Vector3 pos;
        float width;
        float depth;
        Matrix4 proj;

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