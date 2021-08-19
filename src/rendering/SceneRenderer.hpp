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

    // Shadows
    Pipeline* m_ShadowPipeline;
    Texture* m_ShadowMap;
    Texture* m_ShadowDepth;
    Framebuffer* m_ShadowFramebuffer;
    Matrix4 m_ShadowViewProj;
    Vector3 m_ShadowDir;
};

}
