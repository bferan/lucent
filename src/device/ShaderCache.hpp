#pragma once

#include "device/Shader.hpp"

namespace lucent
{

template<typename T, int N>
using SlotList = std::array <std::optional<T>, N>;

struct SetLayout
{
    SlotList<VkDescriptorType, Shader::kMaxBindingsPerSet> bindings;
};

inline bool operator==(const SetLayout& lhs, const SetLayout& rhs)
{
    return lhs.bindings == rhs.bindings;
}

struct PipelineLayout
{
    SlotList<SetLayout, Shader::kMaxSets> sets;
};

inline bool operator==(const PipelineLayout& lhs, const PipelineLayout& rhs)
{
    return lhs.sets == rhs.sets;
}

struct LayoutHash
{
    size_t operator()(const SetLayout& set) const;
    size_t operator()(const PipelineLayout& layout) const;
};

// Currently, specific to the Vulkan backend, could be made general
class ShaderCache
{
public:
    explicit ShaderCache(Device* device);

    Shader* Compile(const std::string& name);

    Shader* Compile(const std::string& name, ShaderInfoLog& log);

    void Release(Shader* shader);

    // Release all resources from the cache
    void Clear();

private:
    bool PopulateShader(const std::string& name, const std::string& source, Shader& shader, ShaderInfoLog& log);
    bool PopulateLayout(const PipelineLayout& layout, Shader& shader, ShaderInfoLog& log);

    void FreeResources(Shader* shader);

    VkDescriptorSetLayout FindSetLayout(const SetLayout& layout);
    VkPipelineLayout FindPipelineLayout(const PipelineLayout& layout);

private:
    Device* m_Device;
    std::vector <uint32_t> m_SpirvBuffer;
    std::unique_ptr <ShaderResolver> m_Resolver;

    std::unordered_map <uint64_t, std::unique_ptr<Shader>> m_Shaders;
    std::unordered_map <SetLayout, VkDescriptorSetLayout, LayoutHash> m_SetLayouts;
    std::unordered_map <PipelineLayout, VkPipelineLayout, LayoutHash> m_PipelineLayouts;
};

}
