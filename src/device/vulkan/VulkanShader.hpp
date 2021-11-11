#pragma once

#include <core/Hash.hpp>

#include "vulkan/vulkan.h"
#include "device/Descriptor.hpp"

namespace lucent
{

struct ShaderInfoLog
{
public:
    void Error(const std::string& text);

public:
    std::string error;
};

enum class ShaderStage
{
    kVertex,
    kFragment,
    kCompute
};

//! Locates shader source from a given shader name
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

struct VulkanShader
{
    static constexpr int kMaxStages = 8;
    static constexpr int kMaxSets = 4;
    static constexpr int kMaxBindingsPerSet = 16;
    static constexpr int kMaxDescriptors = 64;
    static constexpr int kMaxDynamicDescriptorsPerSet = 4;
    static constexpr int kMaxDescriptorBlocks = 8;

    struct Stage
    {
        VkShaderStageFlagBits stageBit;
        VkShaderModule module;
    };

    Array <Stage, kMaxStages> stages;
    Array <VkDescriptorSetLayout, kMaxSets> setLayouts;
    Array <Descriptor, kMaxDescriptors> descriptors;
    Array <Descriptor, kMaxDescriptorBlocks> blocks;
    VkPipelineLayout pipelineLayout{};
    uint64 hash{};
    uint32 uses{};
};

}
