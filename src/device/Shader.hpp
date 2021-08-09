#pragma once

#include "vulkan/vulkan.h"

#include "device/Device.hpp"

namespace lucent
{

struct ShaderInfoLog
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

    VkPipelineLayout layout;
    uint32_t numSets;
    VkDescriptorSetLayout setLayouts[kMaxSets];

    uint64_t hash;
    uint32_t uses;
};

}
