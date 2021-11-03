#pragma once

#include "VulkanShader.hpp"

namespace lucent
{

class VulkanDevice;

// Specific to the Vulkan backend, could be made general
class ShaderCache
{
public:
    explicit ShaderCache(VulkanDevice* device);

    VulkanShader* Compile(const std::string& name);

    VulkanShader* Compile(const std::string& name, ShaderInfoLog& log);

    void Release(VulkanShader* shader);

    // Release all resources from the cache
    void Clear();

public:
    template<typename T, int N>
    using SlotList = std::array<std::optional<T>, N>;

    using SetList = Array<VkDescriptorSetLayout, VulkanShader::kMaxSets>;

    struct SetLayout
    {
        bool operator==(const SetLayout other) const
        {
            return bindings == other.bindings;
        }
        SlotList<VkDescriptorType, VulkanShader::kMaxBindingsPerSet> bindings;
    };

    struct PipelineLayout
    {
        SlotList<SetLayout, VulkanShader::kMaxSets> sets;
    };

    struct LayoutHash
    {
        size_t operator()(const SetLayout& set) const;
        size_t operator()(const SetList& sets) const;
    };

private:
    bool PopulateShaderModules(VulkanShader& shader, const std::string& name, const std::string& source, ShaderInfoLog& log);
    bool PopulateShaderLayout(VulkanShader& shader, const PipelineLayout& layout, ShaderInfoLog& log);

    void FreeResources(VulkanShader* shader);

    VkDescriptorSetLayout FindSetLayout(const SetLayout& layout);

private:
    VulkanDevice* m_Device;
    std::vector<uint32> m_SpirvBuffer;
    std::unique_ptr<ShaderResolver> m_Resolver;
    std::unordered_map<uint64, std::unique_ptr<VulkanShader>> m_Shaders;

    std::unordered_map<SetLayout, VkDescriptorSetLayout, LayoutHash> m_SetLayouts;
    std::unordered_map<SetList, VkPipelineLayout, LayoutHash> m_PipelineLayouts;
};

}
