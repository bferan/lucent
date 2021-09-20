#pragma once

#include <core/Hash.hpp>
#include "vulkan/vulkan.h"

namespace lucent
{

struct DescriptorID
{
    uint32 hash;
    std::string_view name;
};

inline constexpr DescriptorID operator ""_id(const char* name, size_t size)
{
    auto nameView = std::string_view(name, size);
    auto hash = Hash<uint32>(nameView);
    return { hash, nameView };
}

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
struct Descriptor
{
    uint32 hash: 32;
    uint32 set: 2;
    uint32 binding: 4;
    uint32 offset: 16;
    uint32 size: 16;
};
static_assert(sizeof(Descriptor) == 12);

struct Shader
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

    Descriptor* Lookup(DescriptorID id);

    Array <Stage, kMaxStages> stages;
    Array <VkDescriptorSetLayout, kMaxSets> setLayouts;
    Array <Descriptor, kMaxDescriptors> descriptors;
    Array <Descriptor, kMaxDescriptorBlocks> blocks;
    VkPipelineLayout pipelineLayout;
    uint64 hash;
    uint32 uses;
};

}
