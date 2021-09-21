#pragma once

#include "device/Device.hpp"

namespace lucent
{

using RenderPass = std::function<void(Context&)>;

class Renderer
{
public:
    Texture* AddRenderTarget(const TextureSettings& settings);

    Framebuffer* AddFramebuffer(const FramebufferSettings& settings);

    Pipeline* AddPipeline(const PipelineSettings& settings);

    void AddPass(const char* label, RenderPass pass);

    Buffer* GetTransferBuffer();

    void Clear();

    bool Render();

private:
    std::vector<RenderPass> m_RenderPasses;
    std::vector<Texture*> m_RenderTargets;
    std::vector<Framebuffer*> m_Framebuffers;

    std::vector<Context*> m_ContextsPerFrame;
    uint32_t m_FrameIndex;
};

}
