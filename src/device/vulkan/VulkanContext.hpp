#pragma once

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

    friend class VulkanDevice;

private:
    void EstablishBindings();
    VkDescriptorSet FindDescriptorSet(const BindingArray& bindings, VkDescriptorSetLayout layout);
    uint32 GetScratchOffset(uint32 arg, uint32 binding);
    void ResetScratchBindings();
    void BindBuffer(uint32 set, uint32 binding, const Buffer* buffer, uint32 dynamicOffset);

    void TransitionLayout(const Texture* generalTexture, VkPipelineStageFlags stage, VkAccessFlags access,
        VkImageLayout layout, uint32 layer = ~0u, uint32 level = ~0u);

    void RestoreLayout(const Texture* texture, VkPipelineStageFlags stage, VkAccessFlags access,
        VkImageLayout layout, uint32 layer = ~0u, uint32 level = ~0u);

public:
    VulkanDevice& m_Device;

    VkCommandPool m_CommandPool{};
    VkCommandBuffer m_CommandBuffer{};
    VkFence m_ReadyFence{};

    VkDescriptorPool m_DescriptorPool{};
    std::unordered_map<BindingArray, VkDescriptorSet, BindingHash> m_DescriptorSets;

    // Scratch binding
    static constexpr auto kMaxScratchBindings = 8;
    Buffer* m_ScratchUniformBuffer{};
    uint32 m_ScratchDrawOffset = 0;
    Array <ScratchBinding, kMaxScratchBindings> m_ScratchBindings{};

    // Active state
    const VulkanPipeline* m_BoundPipeline{};
    const VulkanFramebuffer* m_BoundFramebuffer{};
    std::array<BoundSet, Shader::kMaxSets> m_BoundSets{};
};

}
