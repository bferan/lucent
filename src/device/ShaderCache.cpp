#include "ShaderCache.hpp"

#include "glslang/Public/ShaderLang.h"
#include "glslang/Include/Types.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include <optional>
#include <array>

#include "core/Lucent.hpp"
#include "core/Hash.hpp"
#include "device/ResourceLimits.hpp"

namespace lucent
{

// Hashing
static uint64_t HashProgramText(const ProgramInfo& info)
{
    auto hash = FNVTraits<uint64_t>::kOffset;

    hash = Hash(info.vertShader, hash);
    hash = Hash(info.fragShader, hash);

    return hash;
}

// Set layout
static VkDescriptorType BasicTypeToDescriptorType(glslang::TBasicType basicType)
{
    switch (basicType)
    {
    case glslang::EbtBlock:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    case glslang::EbtSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:
        LC_ASSERT(0 && "Unsupported uniform type declared in shader");
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

struct PipelineLayout
{
    template<typename T, int N>
    using SlotList = std::array<std::optional<T>, N>;

    struct SetLayout
    {
        SlotList<VkDescriptorType, CompiledProgram::kMaxBindingsPerSet> bindings;
    };

    SlotList<SetLayout, CompiledProgram::kMaxSets> sets;
};

// Shader cache
ShaderCache::ShaderCache(Device* device)
    : m_Device(device)
{
}

CompiledProgram& ShaderCache::Compile(const ProgramInfo& info)
{
    auto hash = HashProgramText(info);

    if (m_Programs.contains(hash))
    {
        return *m_Programs[hash];
    }

    auto& program = *(m_Programs[hash] = std::make_unique<CompiledProgram>());
    PopulateProgram(info, program);
    return program;
}

static void ScanLayout(const TIntermNode& root, PipelineLayout& layout)
{
    for (auto node : root.getAsAggregate()->getSequence())
    {
        auto agg = node->getAsAggregate();
        if (agg->getOp() == glslang::EOpLinkerObjects)
        {
            for (auto obj : agg->getSequence())
            {
                auto symbol = obj->getAsSymbolNode();
                auto& type = symbol->getType();
                auto& qualifier = type.getQualifier();

                if (qualifier.hasUniformLayout())
                {
                    auto set = qualifier.layoutSet;
                    auto binding = qualifier.layoutBinding;

                    if (set >= 0 && set < CompiledProgram::kMaxSets &&
                        binding >= 0 && binding < CompiledProgram::kMaxBindingsPerSet)
                    {
                        if (!layout.sets[set])
                            layout.sets[set] = PipelineLayout::SetLayout{};

                        auto& setLayout = layout.sets[set].value();
                        setLayout.bindings[binding] = BasicTypeToDescriptorType(type.getBasicType());
                    }
                    else
                    {
                        LC_ASSERT(0 && "Invalid uniform binding in shader");
                    }
                }
            }
        }
    }
}

bool ShaderCache::PopulateProgram(const ProgramInfo& info, CompiledProgram& compiled)
{
    const int defaultVersion = 450;
    const auto inputLang = glslang::EShSourceGlsl;
    const auto client = glslang::EShClientVulkan;
    const auto clientVersion = glslang::EShTargetVulkan_1_2;
    const auto targetLang = glslang::EshTargetSpv;
    const auto targetLangVersion = glslang::EShTargetSpv_1_2;

    compiled = {};
    std::vector<std::unique_ptr<glslang::TShader>> shaders;
    glslang::TProgram program;

    auto addShader = [&](std::string_view text, EShLanguage lang, VkShaderStageFlagBits bit)
    {
        if (text.empty()) return;

        auto& shader = *shaders.emplace_back(std::make_unique<glslang::TShader>(lang));
        auto data = text.data();
        int length = (int)text.length();

        shader.setStringsWithLengths(&data, &length, 1);
        shader.setEnvInput(inputLang, lang, client, defaultVersion);
        shader.setEnvClient(client, clientVersion);
        shader.setEnvTarget(targetLang, targetLangVersion);

        shader.parse(&builtInResource, defaultVersion, true, EShMsgDefault);

        program.addShader(&shader);

        auto stage = compiled.numStages;
        compiled.stages[stage].stageBit = bit;
        compiled.numStages++;
    };

    addShader(info.vertShader, EShLangVertex, VK_SHADER_STAGE_VERTEX_BIT);
    addShader(info.fragShader, EShLangFragment, VK_SHADER_STAGE_FRAGMENT_BIT);

    program.link(EShMsgDefault);

    PipelineLayout layout;
    for (int i = 0; i < compiled.numStages; ++i)
    {
        m_SpirvBuffer.clear();

        auto& inter = *shaders[i]->getIntermediate();
        glslang::GlslangToSpv(inter, m_SpirvBuffer);

        auto moduleInfo = VkShaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = m_SpirvBuffer.size() * sizeof(uint32_t),
            .pCode = m_SpirvBuffer.data()
        };

        auto result = vkCreateShaderModule(m_Device->m_Device, &moduleInfo, nullptr, &compiled.stages[i].module);
        LC_ASSERT(result == VK_SUCCESS);

        ScanLayout(*inter.getTreeRoot(), layout);
    }
    PopulateLayout(layout, compiled);

    return true;
}

void ShaderCache::PopulateLayout(const PipelineLayout& layout, CompiledProgram& compiled)
{
    // Create descriptor set layout per set
    for (const auto& set : layout.sets)
    {
        if (set)
        {
            auto& setLayout = set.value();

            uint32_t numBindings = 0;
            VkDescriptorSetLayoutBinding bindings[CompiledProgram::kMaxBindingsPerSet];
            for (uint32_t binding = 0; binding < setLayout.bindings.size(); ++binding)
            {
                if (setLayout.bindings[binding])
                {
                    bindings[numBindings] = VkDescriptorSetLayoutBinding{
                        .binding = binding,
                        .descriptorType = setLayout.bindings[binding].value(),
                        .descriptorCount = 1,
                        .stageFlags = VK_SHADER_STAGE_ALL
                    };
                    numBindings++;
                }
            }

            auto setLayoutInfo = VkDescriptorSetLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = numBindings,
                .pBindings = bindings
            };

            auto result = vkCreateDescriptorSetLayout(m_Device->m_Device,
                &setLayoutInfo,
                nullptr,
                &compiled.setLayouts[compiled.numSets]);
            LC_ASSERT(result == VK_SUCCESS);
            compiled.numSets++;
        }
    }

    // Create pipeline layout
    auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = compiled.numSets,
        .pSetLayouts = compiled.setLayouts
    };

    auto result = vkCreatePipelineLayout(m_Device->m_Device, &pipelineLayoutInfo, nullptr, &compiled.layout);
    LC_ASSERT(result == VK_SUCCESS);
}

void ShaderCache::Clear()
{
    for (auto&[_, program] : m_Programs)
    {
        for (int i = 0; i < program->numStages; ++i)
        {
            vkDestroyShaderModule(m_Device->m_Device, program->stages[i].module, nullptr);
        }

        vkDestroyPipelineLayout(m_Device->m_Device, program->layout, nullptr);

        for (int i = 0; i < program->numSets; ++i)
        {
            vkDestroyDescriptorSetLayout(m_Device->m_Device, program->setLayouts[i], nullptr);
        }
    }
}

}
