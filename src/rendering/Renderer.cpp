#include "Renderer.hpp"

#include "device/Context.hpp"

namespace lucent
{

Renderer::Renderer(Device* device, const RenderSettings& settings)
    : m_Device(device)
    , m_Settings(settings)
    , m_FrameIndex(0)
{
    m_TransferBuffer = m_Device->CreateBuffer(BufferType::kStaging, 64 * 1024 * 1024);

    for (int i = 0; i < settings.framesInFlight; ++i)
    {
        m_ContextsPerFrame.push_back(m_Device->CreateContext());
    }
}

Texture* Renderer::AddRenderTarget(const TextureSettings& settings)
{
    return m_RenderTargets.emplace_back(m_Device->CreateTexture(settings));
}

Framebuffer* Renderer::AddFramebuffer(const FramebufferSettings& settings)
{
    return m_Framebuffers.emplace_back(m_Device->CreateFramebuffer(settings));
}

Pipeline* Renderer::AddPipeline(const PipelineSettings& settings)
{
    return m_Pipelines.emplace_back(m_Device->CreatePipeline(settings));
}

void Renderer::AddPass(const char* label, RenderPass pass)
{
    m_RenderPasses.push_back(std::move(pass));
}

void Renderer::AddPresentPass(Texture* presentSrc)
{
    m_PresentSrc = presentSrc;
}

Buffer* Renderer::GetTransferBuffer()
{
    return m_TransferBuffer;
}

const RenderSettings& Renderer::GetSettings()
{
    return m_Settings;
}

void Renderer::Clear()
{
    m_Device->WaitIdle();

    m_RenderPasses.clear();

    for (auto texture: m_RenderTargets)
        m_Device->DestroyTexture(texture);
    m_RenderTargets.clear();

    for (auto framebuffer: m_Framebuffers)
        m_Device->DestroyFramebuffer(framebuffer);
    m_Framebuffers.clear();

    for (auto pipeline: m_Pipelines)
        m_Device->DestroyPipeline(pipeline);
    m_Pipelines.clear();
}

bool Renderer::Render(Scene& scene)
{
    auto& ctx = *m_ContextsPerFrame[m_FrameIndex % m_Settings.framesInFlight];
    auto device = ctx.GetDevice();

    auto target = device->AcquireSwapchainImage();

    ctx.Begin();

    for (auto& pass: m_RenderPasses)
    {
        pass(ctx, scene);
    }

    ctx.BlitTexture(m_PresentSrc, 0, 0, target, 0, 0);

    ctx.End();

    m_Device->Submit(&ctx);
    auto success = m_Device->Present();

    ++m_FrameIndex;

    return success;
}

void Renderer::SetSettings(const RenderSettings& settings)
{
    m_Settings = settings;
}

}
