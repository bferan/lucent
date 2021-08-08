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

struct InfoLog
{
public:
    void Error(const std::string& text);

public:
    std::string error;
};

class ShaderResolver
{
public:
    struct Result
    {
        bool found;
        std::string source;
        std::string qualifiedName;
    };

public:
    virtual Result Resolve(const std::string& name) = 0;

    virtual ~ShaderResolver() = default;;
};

struct Shader
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

    uint64_t hash;
    uint32_t uses;
};

// Currently, specific to the Vulkan backend, could be made general
class ShaderCache
{
public:
    explicit ShaderCache(Device* device);

    Shader* Compile(const std::string& name);

    Shader* Compile(const std::string& name, InfoLog& log);

    void Release(Shader* shader);

    // Release all resources from the cache
    void Clear();

private:
    bool PopulateShader(const std::string& name, const std::string& source, Shader& shader, InfoLog& log);
    bool PopulateLayout(const PipelineLayout& layout, Shader& shader, InfoLog& log);
    void FreeResources(Shader* shader);

private:
    Device* m_Device;
    std::vector<uint32_t> m_SpirvBuffer;
    std::unique_ptr<ShaderResolver> m_Resolver;

    std::unordered_map<uint64_t, VkPipelineLayout> m_PipelineLayouts;
    std::unordered_map<uint64_t, VkDescriptorSetLayout> m_DescLayouts;
    std::unordered_map<uint64_t, std::unique_ptr<Shader>> m_Shaders;
};

}
