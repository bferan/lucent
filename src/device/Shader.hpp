#pragma once

#include <core/Hash.hpp>
#include "vulkan/vulkan.h"

namespace lucent
{

using DescriptorID = uint32;

inline constexpr DescriptorID operator ""_id(const char* name)
{
    return Hash<DescriptorID>(name);
}

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

    virtual ~ShaderResolver() = default;
};

// Binary search these
struct DescriptorEntry
{
    uint32 hash: 32;
    uint32 set: 2;
    uint32 binding: 4;
};

struct Shader
{
    static constexpr int kMaxStages = 8;
    static constexpr int kMaxSets = 4;
    static constexpr int kMaxBindingsPerSet = 16;
    static constexpr int kMaxDescriptors = 64;
    static constexpr int kMaxDynamicDescriptorsPerSet = 4;

    struct Stage
    {
        VkShaderStageFlagBits stageBit;
        VkShaderModule module;
    };

    Array <Stage, kMaxStages> stages;
    Array <VkDescriptorSetLayout, kMaxSets> setLayouts;
    Array <DescriptorEntry, kMaxDescriptors> descriptors;
    VkPipelineLayout pipelineLayout;
    uint64 hash;
    uint32 uses;
};

}
