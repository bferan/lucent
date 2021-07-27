#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <string_view>

#include "vulkan/vulkan.h"

#include "device/Device.hpp"

namespace lucent
{

struct PipelineLayout;

struct CompiledProgram
{
    static constexpr int kMaxStages = 8;
    static constexpr int kMaxSets = 4;
    static constexpr int kMaxBindingsPerSet = 16;

    struct Stage
    {
        VkShaderStageFlagBits stageBit;
        VkShaderModule module;
    };

    uint32_t numStages;
    Stage stages[kMaxStages];

    uint32_t numSets;
    VkDescriptorSetLayout setLayouts[kMaxSets];

    VkPipelineLayout layout;

    std::vector<std::string> includes;
};

// Currently specific to the Vulkan backend, could be made general
class ShaderCache
{
public:
    explicit ShaderCache(Device* device);

    CompiledProgram& Compile(const std::string& name, const std::string& source);

    // Release all resources from the cache
    void Clear();

private:
    bool PopulateProgram(const std::string& name, const std::string& source, CompiledProgram& compiled);
    void PopulateLayout(const PipelineLayout& layout, CompiledProgram& compiled);

private:
    Device* m_Device;
    std::vector<uint32_t> m_SpirvBuffer;

    std::unordered_map<uint64_t, VkPipelineLayout> m_PipelineLayouts;
    std::unordered_map<uint64_t, VkDescriptorSetLayout> m_DescLayouts;
    std::unordered_map<uint64_t, std::unique_ptr<CompiledProgram>> m_Programs;
};

}
