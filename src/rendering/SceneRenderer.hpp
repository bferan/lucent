#pragma once

#include "device/Device.hpp"
#include "scene/Scene.hpp"
#include "debug/DebugConsole.hpp"

namespace lucent
{

struct alignas(16) DebugShape
{
    Color color;
    Vector3 srcPos;
    float r;
    Vector3 dstPos;
    uint32 type;
};

const int kNumDebugShapes = 1024;

struct DebugShapeBuffer
{
    uint32 numShapes = 0;
    DebugShape shapes[kNumDebugShapes];
};

class SceneRenderer
{
public:
    explicit SceneRenderer(Device* device);

    void Render(Scene& scene);

private:
    Texture* GenerateHaltonNoiseTexture(uint32 size);

    void RenderShadows(Scene& scene);

    void RenderGBuffer(Scene& scene, Matrix4 view, Matrix4 proj) const;

    void RenderAmbientOcclusion(Scene& scene, Matrix4 view, Matrix4 proj, Matrix4 invView) const;

    void RenderReflections(Scene& scene, Matrix4 proj, Matrix4 invView) const;

    void RenderDebugOverlay(Scene& scene, Matrix4 view, Matrix4 proj) const;

    void CalculateCascades(Scene& scene);

public:
    Device* m_Device;
    Context* m_Context;

    Pipeline* m_DefaultPipeline;
    Pipeline* m_SkyboxPipeline;

    Texture* m_SceneColor;
    Framebuffer* m_SceneColorFramebuffer;

    // TEMP DEBUG
    std::unique_ptr<DebugConsole> m_DebugConsole;
    Buffer* m_DebugShapeBuffer;
    DebugShapeBuffer* m_DebugShapes;
    Pipeline* m_DebugShapePipeline;

    // Moment shadows
    static constexpr int kShadowTargets = DirectionalLight::kNumCascades;
    Texture* m_MomentMap;
    Texture* m_MomentTempDepth;
    Array<Framebuffer*, kShadowTargets> m_MomentMapTargets;
    Array<Texture*, kShadowTargets> m_MomentDepthTextures;
    Array<Framebuffer*, kShadowTargets> m_MomentDepthTargets;
    Pipeline* m_MomentDepthOnly;
    Pipeline* m_MomentResolveDepth;

    Matrix4 m_ShadowViewProj;
    Matrix4 m_ShadowViewProj1;
    Vector3 m_ShadowDir;
    Vector4 m_ShadowPlane[3];
    Vector4 m_ShadowScale[3];
    Vector4 m_ShadowOffset[3];

    // AO
    Texture* m_AOMap;
    Pipeline* m_ComputeAO;

    // GBuffer
    Texture* m_GBaseColor;
    Texture* m_GDepth;
    Texture* m_GNormals;
    Texture* m_GMetalRoughness;
    Framebuffer* m_GFramebuffer;
    Texture* m_GMinZ;
    Pipeline* m_GenGBuffer;
    Pipeline* m_GenMinZ;

    // SSR
    Texture* m_BlueNoise;
    Texture* m_ReflectionResult;
    Texture* m_ReflectionResolve;
    Pipeline* m_TraceReflections;
    Pipeline* m_ResolveReflections;

    Buffer* m_TransferBuffer;


};

}
