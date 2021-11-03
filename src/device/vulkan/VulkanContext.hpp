#pragma once

#include "core/Pool.hpp"
#include "device/Context.hpp"
#include "device/vulkan/VulkanDevice.hpp"
#include "VulkanCommon.hpp"

namespace lucent
{

class VulkanContext : public Context
{
public:
    explicit VulkanContext(VulkanDevice& device);
    ~VulkanContext();

    VulkanContext(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    void Begin() override;
    void End() override;

    void BeginRenderPass(const Framebuffer* framebuffer) override;
    void EndRenderPass() override;

    void Clear(Color color, float depth) override;
    void Viewport(uint32 width, uint32 height) override;

    void BindPipeline(const Pipeline* pipeline) override;
    void BindBuffer(const Buffer* buffer) override;

    void BindBuffer(Descriptor* descriptor, const Buffer* buffer) override;
    void BindBuffer(Descriptor* descriptor, const Buffer* buffer, uint32 dynamicOffset) override;
    void BindTexture(Descriptor* descriptor, const Texture* texture, int level = -1) override;
    void BindImage(Descriptor* descriptor, const Texture* texture, int level = -1) override;

    void Uniform(Descriptor* descriptor, const uint8* data, size_t size) override;
    void Uniform(Descriptor* descriptor, uint32 arrayIndex, const uint8* data, size_t size) override;

    void Draw(uint32 indexCount) override;

    void Dispatch(uint32 x, uint32 y, uint32 z) override;

    void CopyTexture(
        Texture* src, uint32 srcLayer, uint32 srcLevel,
        Texture* dst, uint32 dstLayer, uint32 dstLevel,
        uint32 width, uint32 height) override;

    void CopyTexture(
        Texture* src, uint32 srcLayer, uint32 srcLevel,
        Buffer* dst, uint32 offset,
        uint32 width, uint32 height) override;

    void CopyTexture(
        Buffer* src, uint32 offset,
        Texture* dst, uint32 dstLayer, uint32 dstLevel,
        uint32 width, uint32 height) override;

    void BlitTexture(
        Texture* src, uint32 srcLayer, uint32 srcLevel,
        Texture* dst, uint32 dstLayer, uint32 dstLevel) override;

    void GenerateMips(Texture* texture) override;

    const Pipeline* BoundPipeline() override;

    Device* GetDevice() override;

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
            const VulkanBuffer* buffer;
            const VulkanTexture* texture;
        };
        int level = -1;

        bool operator==(const Binding& rhs) const;
    };

    using BindingArray = std::array<Binding, VulkanShader::kMaxBindingsPerSet>;

    // Represents an actively bound set of descriptors
    struct BoundSet
    {
        BindingArray bindings;
        Array <uint32, VulkanShader::kMaxDynamicDescriptorsPerSet> dynamicOffsets;
        bool dirty = false;
    };

    struct BindingHash
    {
        size_t operator()(const BindingArray& bindings) const;
    };

    struct ScratchAllocation
    {
        uint32 set;
        uint32 binding;
        uint32 offset;
        uint32 size;
    };

    friend class VulkanDevice;

private:
    VkDescriptorSet FindDescriptorSet(const BindingArray& bindings, VkDescriptorSetLayout layout);
    VkDescriptorPool AllocateDescriptorPool() const;
    void BindDescriptorSets();

    uint32 GetUniformBufferOffset(uint32 arg, uint32 binding);
    Buffer* AllocateUniformBuffer();
    void ResetScratchAllocations();
    void ResetUniformBuffers();

    void BindBuffer(uint32 set, uint32 binding, const Buffer* buffer, uint32 dynamicOffset);

    void TransitionLayout(const Texture* generalTexture, VkPipelineStageFlags stage, VkAccessFlags access,
        VkImageLayout layout, uint32 layer = ~0u, uint32 level = ~0u) const;

    void RestoreLayout(const Texture* texture, VkPipelineStageFlags stage, VkAccessFlags access,
        VkImageLayout layout, uint32 layer = ~0u, uint32 level = ~0u) const;

public:
    VulkanDevice& m_Device;

    VkCommandPool m_CommandPool{};
    VkCommandBuffer m_CommandBuffer{};
    VkFence m_ReadyFence{};

    Pool<VkDescriptorPool> m_DescriptorPools;
    std::unordered_map<BindingArray, VkDescriptorSet, BindingHash> m_DescriptorSets;

    // Scratch uniform allocations
    static constexpr auto kMaxScratchAllocations = 8;
    static constexpr auto kScratchBufferSize = 65536;

    Pool<Buffer*> m_ScratchUniformBuffers;
    uint32 m_ScratchDrawOffset = 0;
    Array <ScratchAllocation, kMaxScratchAllocations> m_ScratchAllocations{};

    // Active state
    const VulkanPipeline* m_BoundPipeline{};
    const VulkanFramebuffer* m_BoundFramebuffer{};
    std::array<BoundSet, VulkanShader::kMaxSets> m_BoundSets{};
};

}
