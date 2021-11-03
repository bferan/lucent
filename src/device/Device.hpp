#pragma once

#include "device/vulkan/VulkanShader.hpp"
#include "device/Texture.hpp"
#include "device/Framebuffer.hpp"
#include "device/Pipeline.hpp"
#include "device/Buffer.hpp"

#include "debug/Input.hpp"

namespace lucent
{

class Context;

class Device
{
public:
    virtual Buffer* CreateBuffer(BufferType type, size_t size) = 0;
    virtual void DestroyBuffer(Buffer* buffer) = 0;

    virtual Texture* CreateTexture(const TextureSettings& textureSettings) = 0;
    virtual void DestroyTexture(Texture* texture) = 0;

    virtual Pipeline* CreatePipeline(const PipelineSettings& pipelineSettings) = 0;
    virtual void DestroyPipeline(Pipeline* pipeline) = 0;
    virtual void ReloadPipelines() = 0;

    virtual Framebuffer* CreateFramebuffer(const FramebufferSettings& framebufferSettings) = 0;
    virtual void DestroyFramebuffer(Framebuffer* framebuffer) = 0;

    virtual Context* CreateContext() = 0;
    virtual void DestroyContext(Context* context) = 0;

    virtual void Submit(Context* context) = 0;

    virtual Texture* AcquireSwapchainImage() = 0;
    virtual bool Present() = 0;

    virtual void WaitIdle() = 0;
    virtual void RebuildSwapchain() = 0;

    virtual ~Device() = default;
};

}
