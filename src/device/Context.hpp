#pragma once

#include "vulkan/vulkan.h"

#include "device/Device.hpp"

namespace lucent
{

class Context
{
public:
    explicit Context(Device& device);
    void Dispose();

    void Begin() const;
    void End() const;

    void BeginRenderPass(const Framebuffer& fbuffer, VkExtent2D extent = {});
    void EndRenderPass() const;

    void Bind(const Pipeline* pipeline);
    void Bind(const Buffer* buffer);

    void Bind(uint32 set, uint32 binding, const Buffer* buffer);
    void Bind(uint32 set, uint32 binding, const Buffer* buffer, uint32 dynamicOffset);
    void Bind(uint32 set, uint32 binding, const Texture* texture);

    void Draw(uint32 indexCount);

    void CopyTexture(
        Texture* src, int srcLayer, int srcLevel,
        Texture* dst, int dstLayer, int dstLevel,
        uint32 width, uint32 height);

private:
    using NullBinding = std::monostate;
    using BufferBinding = const Buffer*;
    using TextureBinding = const Texture*;
    using Binding = std::variant<NullBinding, BufferBinding, TextureBinding>;
    using BindingArray = std::array<Binding, Shader::kMaxBindingsPerSet>;

    struct BoundSet
    {
        BindingArray bindings;
        bool dirty = false;
        Array<uint32, Shader::kMaxDynamicDescriptorsPerSet> dynamicOffsets;
    };

    struct BindingHash
    {
        size_t operator()(const BindingArray& bindings) const;
    };

    std::array<BoundSet, Shader::kMaxSets> m_BoundSets{};
    std::unordered_map<BindingArray, VkDescriptorSet, BindingHash> m_DescriptorSets;

private:
    void EstablishBindings();
    VkDescriptorSet FindDescriptorSet(const BindingArray& bindings, VkDescriptorSetLayout layout);

public:
    Device& m_Device;

    VkCommandPool m_CommandPool{};
    VkCommandBuffer m_CommandBuffer{};

    VkDescriptorPool m_DescriptorPool{};

    // Active state
    bool m_SwapchainWritten{};
    const Pipeline* m_BoundPipeline{};
};

}
