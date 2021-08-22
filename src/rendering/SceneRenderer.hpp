#pragma once

#include "device/Device.hpp"
#include "scene/Scene.hpp"
#include "debug/DebugConsole.hpp"

namespace lucent
{

class SceneRenderer
{
public:
    explicit SceneRenderer(Device* device);

    void Render(Scene& scene);

private:
    void RenderShadows(Scene& scene);

    void CalculateCascades(Scene& scene);

public:
    Device* m_Device;
    Context* m_Context;

    Pipeline* m_DefaultPipeline;
    Pipeline* m_SkyboxPipeline;

    // TEMP
    std::unique_ptr<DebugConsole> m_DebugConsole;

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
};

}
