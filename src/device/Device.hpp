#pragma once

#include "device/Shader.hpp"
#include "device/Texture.hpp"
#include "device/Framebuffer.hpp"
#include "device/Pipeline.hpp"
#include "device/Buffer.hpp"

#include "debug/Input.hpp"

namespace lucent
{

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector4 tangent;
    Vector2 texCoord0;
    Color color = Color::White();
};

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

    virtual const Framebuffer* AcquireFramebuffer() = 0;

    virtual Context* CreateContext() = 0;
    virtual void DestroyContext(Context* context) = 0;

    virtual void Submit(Context* context, bool sync = true) = 0;

    virtual void Present() = 0;

    virtual ~Device() = default;

public:
    std::unique_ptr<Input> m_Input;
};

}
