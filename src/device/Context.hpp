#pragma once

#include "device/Device.hpp"
#include "device/vulkan/VulkanShader.hpp"

namespace lucent
{

//! Abstracts an underlying command buffer, allowing you to bind resources and execute rendering commands
class Context
{
public:
    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void BeginRenderPass(const Framebuffer* framebuffer) = 0;
    virtual void EndRenderPass() = 0;

    virtual void Clear(Color color = Color::Black(), float depth = 1.0f) = 0;
    virtual void Viewport(uint32 width, uint32 height) = 0;

    virtual void BindPipeline(const Pipeline* pipeline) = 0;
    virtual void BindBuffer(const Buffer* buffer) = 0;

    virtual void BindBuffer(Descriptor* descriptor, const Buffer* buffer) = 0;
    void BindBuffer(DescriptorID id, const Buffer* buffer);

    virtual void BindBuffer(Descriptor* descriptor, const Buffer* buffer, uint32 dynamicOffset) = 0;
    void BindBuffer(DescriptorID descriptor, const Buffer* buffer, uint32 dynamicOffset);

    virtual void BindTexture(Descriptor* descriptor, const Texture* texture, int level = -1) = 0;
    void BindTexture(DescriptorID descriptor, const Texture* texture, int level = -1);

    virtual void BindImage(Descriptor* descriptor, const Texture* texture, int level = -1) = 0;
    void BindImage(DescriptorID descriptor, const Texture* texture, int level = -1);

    template<typename T>
    void Uniform(DescriptorID id, const T& value);
    virtual void Uniform(Descriptor* descriptor, const uint8* data, size_t size) = 0;

    template<typename T>
    void Uniform(DescriptorID id, uint32 arrayIndex, const T& value);
    virtual void Uniform(Descriptor* descriptor, uint32 arrayIndex, const uint8* data, size_t size) = 0;

    virtual void Draw(uint32 indexCount) = 0;

    virtual void Dispatch(uint32 x, uint32 y, uint32 z) = 0;

    virtual void CopyTexture(
        Texture* src, uint32 srcLayer, uint32 srcLevel,
        Texture* dst, uint32 dstLayer, uint32 dstLevel,
        uint32 width, uint32 height) = 0;

    virtual void CopyTexture(
        Texture* src, uint32 srcLayer, uint32 srcLevel,
        Buffer* dst, uint32 offset,
        uint32 width, uint32 height) = 0;

    virtual void CopyTexture(
        Buffer* src, uint32 offset,
        Texture* dst, uint32 dstLayer, uint32 dstLevel,
        uint32 width, uint32 height) = 0;

    virtual void BlitTexture(
        Texture* src, uint32 srcLayer, uint32 srcLevel,
        Texture* dst, uint32 dstLayer, uint32 dstLevel) = 0;

    virtual void GenerateMips(Texture* texture) = 0;

    virtual const Pipeline* BoundPipeline() = 0;

    virtual Device* GetDevice() = 0;
};

template<typename T>
void Context::Uniform(DescriptorID id, const T& value)
{
    Uniform(BoundPipeline()->Lookup(id), (const uint8*)&value, sizeof(value));
}

template<typename T>
void Context::Uniform(DescriptorID id, uint32 arrayIndex, const T& value)
{
    Uniform(BoundPipeline()->Lookup(id), arrayIndex, (const uint8*)&value, sizeof(value));
}

}
