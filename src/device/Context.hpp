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

    void Begin();
    void End();

    void BeginRenderPass(const Framebuffer& fbuffer, VkExtent2D extent = {});
    void EndRenderPass() const;

    void Bind(const Pipeline* pipeline);
    void Bind(const Buffer* buffer);

    void Bind(uint32 set, uint32 binding, const Buffer* buffer);
    void Bind(uint32 set, uint32 binding, const Buffer* buffer, uint32 dynamicOffset);
    void Bind(uint32 set, uint32 binding, const Texture* texture);

    void Bind(DescriptorID id, const Buffer* buffer);
    void Bind(DescriptorID id, const Buffer* buffer, uint32 dynamicOffset);
    void Bind(DescriptorID id, const Texture* texture);

    template<typename T>
    void Uniform(DescriptorID id, const T& value);
    void Uniform(DescriptorID id, const uint8* data, size_t size);

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

private:
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
    std::array<BoundSet, Shader::kMaxSets> m_BoundSets{};
};

template<typename T>
void Context::Uniform(DescriptorID id, const T& value)
{
    Uniform(id, (const uint8*)&value, sizeof(value));
}

}
