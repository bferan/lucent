#pragma once

#include "device/Device.hpp"
#include "rendering/RenderSettings.hpp"
#include "rendering/View.hpp"
#include "scene/Scene.hpp"

namespace lucent
{

using RenderPass = std::function<void(Context&, View&)>;

//! Manages a set of render passes and render targets
//! Allows for render passes to be expressed as stateless functions which
//! add data and functors to be executed later.
class Renderer
{
public:
    Renderer(Device* device, RenderSettings settings);

    Texture* AddRenderTarget(const TextureSettings& settings);

    Framebuffer* AddFramebuffer(const FramebufferSettings& settings);

    Pipeline* AddPipeline(const PipelineSettings& settings);

    void AddPass(const char* label, RenderPass pass);

    void AddPresentPass(Texture* presentSrc);

    Buffer* GetTransferBuffer();
    Buffer* GetDebugShapesBuffer();

    RenderSettings& GetSettings();

    void Clear();

    bool Render(Scene& scene);

private:
    Device* m_Device;
    Buffer* m_TransferBuffer;
    Buffer* m_DebugShapesBuffer;

    RenderSettings m_Settings;
    std::vector<RenderPass> m_RenderPasses;
    std::vector<Texture*> m_RenderTargets;
    std::vector<Framebuffer*> m_Framebuffers;
    std::vector<Pipeline*> m_Pipelines;
    std::vector<Context*> m_ContextsPerFrame;
    Texture* m_PresentSrc{};
    uint32_t m_FrameIndex;

    View m_View;
};

}
