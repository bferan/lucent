#pragma once

#include "device/Shader.hpp"

namespace lucent
{

class Device;

// Specific to the Vulkan backend, could be made general
class ShaderCache
{
public:
    explicit ShaderCache(Device* device);

    Shader* Compile(const std::string& name);

    Shader* Compile(const std::string& name, ShaderInfoLog& log);

    void Release(Shader* shader);

    // Release all resources from the cache
    void Clear();

public:
    template<typename T, int N>
    using SlotList = std::array<std::optional<T>, N>;

    using SetList = Array<VkDescriptorSetLayout, Shader::kMaxSets>;

    struct SetLayout
    {
        bool operator==(const SetLayout other) const
        {
            return bindings == other.bindings;
        }
        SlotList<VkDescriptorType, Shader::kMaxBindingsPerSet> bindings;
    };

    struct PipelineLayout
    {
        SlotList<SetLayout, Shader::kMaxSets> sets;
    };

    struct LayoutHash
    {
        size_t operator()(const SetLayout& set) const;
        size_t operator()(const SetList& sets) const;
    };

private:
    bool PopulateShaderModules(Shader& shader, const std::string& name, const std::string& source, ShaderInfoLog& log);
    bool PopulateShaderLayout(Shader& shader, const PipelineLayout& layout, ShaderInfoLog& log);

    void FreeResources(Shader* shader);

    VkDescriptorSetLayout FindSetLayout(const SetLayout& layout);

private:
    Device* m_Device;
    std::vector<uint32> m_SpirvBuffer;
    std::unique_ptr<ShaderResolver> m_Resolver;
    std::unordered_map<uint64, std::unique_ptr<Shader>> m_Shaders;

    std::unordered_map<SetLayout, VkDescriptorSetLayout, LayoutHash> m_SetLayouts;
    std::unordered_map<SetList, VkPipelineLayout, LayoutHash> m_PipelineLayouts;
};

}
