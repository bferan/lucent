#pragma once

#include "vulkan/vulkan.h"

#include "device/Device.hpp"

namespace lucent
{

class Context
{
public:
    explicit Context(Device& device);

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void Dispose();

    void Begin();
    void End();

    void BeginRenderPass(const Framebuffer* framebuffer, VkExtent2D extent = {});
    void EndRenderPass() const;

    void Clear(Color color = Color::Black(), float depth = 1.0f);

    void Bind(const Pipeline* pipeline);
    void Bind(const Buffer* buffer);

    void BindBuffer(uint32 set, uint32 binding, const Buffer* buffer);
    void BindBuffer(DescriptorID id, const Buffer* buffer);

    void BindBuffer(uint32 set, uint32 binding, const Buffer* buffer, uint32 dynamicOffset);
    void BindBuffer(DescriptorID id, const Buffer* buffer, uint32 dynamicOffset);

    void BindTexture(uint32 set, uint32 binding, const Texture* texture, int level = -1);
    void BindTexture(DescriptorID id, const Texture* texture, int level = -1);

    void BindImage(uint32 set, uint32 binding, const Texture* texture, int level = -1);
    void BindImage(DescriptorID id, const Texture* texture, int level = -1);

    template<typename T>
    void Uniform(DescriptorID id, const T& value);
    void Uniform(DescriptorID id, const uint8* data, size_t size);

    template<typename T>
    void Uniform(DescriptorID id, uint32 arrayIndex, const T& value);
    void Uniform(DescriptorID id, uint32 arrayIndex, const uint8* data, size_t size);

    void Draw(uint32 indexCount);

    void Dispatch(uint32 x, uint32 y, uint32 z);

    void CopyTexture(
        Texture* src, int srcLayer, int srcLevel,
        Texture* dst, int dstLayer, int dstLevel,
        uint32 width, uint32 height);

    void AttachmentToImage(Texture* texture);
    void ImageToAttachment(Texture* texture);
    void ImageToGeneral(Texture* texture);
    void ComputeBarrier(Texture* texture, Buffer* buffer);

    void DepthAttachmentToImage(Texture* texture);
    void DepthImageToAttachment(Texture* texture);

    void Transition(Texture* texture,
        VkPipelineStageFlags srcStage, VkAccessFlags srcAccess,
        VkPipelineStageFlags dstStage, VkAccessFlags dstAccess,
        VkImageLayout oldLayout, VkImageLayout newLayout, int level = -1);

private:
    struct Binding
    {
        enum Type
        {
            kNone,
            kUniformBuffer,
            kUniformBufferDynamic,
            kStorageBuffer,
            kTexture,
            kImage
        };

        Type type;
        union
        {
            const void* data;
            const Buffer* buffer;
            const Texture* texture;
        };
        int level = -1;

        bool operator==(const Binding& rhs) const;
    };

    using BindingArray = std::array<Binding, Shader::kMaxBindingsPerSet>;

    // Represents an actively bound set of descriptors
    struct BoundSet
    {
        BindingArray bindings;
        Array <uint32, Shader::kMaxDynamicDescriptorsPerSet> dynamicOffsets;
        bool dirty = false;
    };

    struct BindingHash
    {
        size_t operator()(const BindingArray& bindings) const;
    };

    struct ScratchBinding
    {
        uint32 set;
        uint32 binding;
        uint32 offset;
        uint32 size;
    };

    friend class Device;

private:
    void EstablishBindings();
    VkDescriptorSet FindDescriptorSet(const BindingArray& bindings, VkDescriptorSetLayout layout);
    DescriptorEntry FindDescriptorEntry(DescriptorID id);
    uint32 GetScratchOffset(uint32 arg, uint32 binding);
    void ResetScratchBindings();

public:
    Device& m_Device;

    VkCommandPool m_CommandPool{};
    VkCommandBuffer m_CommandBuffer{};

    VkDescriptorPool m_DescriptorPool{};
    std::unordered_map<BindingArray, VkDescriptorSet, BindingHash> m_DescriptorSets;

    static constexpr auto kMaxScratchBindings = 8;
    Buffer* m_ScratchUniformBuffer{};
    uint32 m_ScratchDrawOffset = 0;
    Array <ScratchBinding, kMaxScratchBindings> m_ScratchBindings{};

    // Active state
    bool m_SwapchainWritten{};
    const Pipeline* m_BoundPipeline{};
    const Framebuffer* m_BoundFramebuffer{};
    std::array<BoundSet, Shader::kMaxSets> m_BoundSets{};
};

template<typename T>
void Context::Uniform(DescriptorID id, const T& value)
{
    Uniform(id, (const uint8*)&value, sizeof(value));
}

template<typename T>
void Context::Uniform(DescriptorID id, uint32 arrayIndex, const T& value)
{
    Uniform(id, arrayIndex, (const uint8*)&value, sizeof(value));
}

}
