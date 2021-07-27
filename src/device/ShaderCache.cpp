#include "ShaderCache.hpp"

#include "glslang/Public/ShaderLang.h"
#include "glslang/Include/Types.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include <optional>
#include <array>

#include "core/Lucent.hpp"
#include "core/Log.hpp"
#include "core/Hash.hpp"
#include "device/ResourceLimits.hpp"

namespace lucent
{

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

// Shader stripping
constexpr auto kVertexDefinition = "void vert()";
constexpr auto kFragmentDefinition = "void frag()";
constexpr auto kComputeDefinition = "void compute()";
constexpr auto kMainDefinition = "void main()";

static void ReplaceAll(std::string& text, const std::string& src, const std::string& dst)
{
    size_t pos = 0;
    while ((pos = text.find(src, pos)) != std::string::npos)
    {
        text.replace(pos, src.length(), dst);
    }
}

static void StripFunction(std::string& text, const std::string& prototype)
{
    auto start = text.find(prototype);
    if (start == std::string::npos) return;

    auto openBracket = text.find('{', start);

    // Find end of function definition
    auto pos = openBracket + 1;
    int nesting = 0;
    int lines = 1;
    while (pos < text.size())
    {
        auto c = text[pos];
        if (c == '/')
        {
            auto next = (pos + 1 < text.size()) ? text[pos + 1] : 0;
            if (next == '/')
            {
                // Skip line comment
                pos = text.find('\n', pos);
                ++lines;
            }
            else if (next == '*')
            {
                // Skip block comment
                pos = text.find("*/", pos + 2) + 1;
            }
        }
        else if (c == '}')
        {
            if (nesting == 0) break;
            --nesting;
        }
        else if (c == '{')
        {
            ++nesting;
        }
        else if (c == '\n')
        {
            ++lines;
        }
        ++pos;
    }

    if (pos >= text.size() || nesting != 0 || text[pos] != '}')
        return;

    text.replace(start, pos - start + 1, lines, '\n');
}

static void StripDeclarations(std::string& text, const std::string& keyword)
{
    size_t pos = 0;
    while ((pos = text.find(keyword, pos)) != std::string::npos)
    {
        // Seek to end of prev statement
        auto prevStatementEnd = text.rfind(';', pos);
        if (prevStatementEnd == std::string::npos) prevStatementEnd = 0;

        auto decStart = text.rfind("layout", pos);
        auto decEnd = text.find(';', pos);

        // Check layout qualifier exists and does not cross statement boundary
        if (decStart == std::string::npos || decStart < prevStatementEnd)
        {
            pos = decEnd;
            continue;
        }
        text.erase(decStart, decEnd - decStart + 1);
        pos = decStart;
    }
}

static void StripShader(std::string& text, ProgramStage stage)
{
    switch (stage)
    {
    case ProgramStage::kVertex:
    {
        StripFunction(text, kFragmentDefinition);
        StripDeclarations(text, "out ");
        ReplaceAll(text, "attribute ", "in ");
        ReplaceAll(text, "varying ", "out ");
        ReplaceAll(text, kVertexDefinition, kMainDefinition);
        break;
    }
    case ProgramStage::kFragment:
    {
        StripFunction(text, kVertexDefinition);
        StripDeclarations(text, "attribute ");
        ReplaceAll(text, "varying ", "in ");
        ReplaceAll(text, kFragmentDefinition, kMainDefinition);
        break;
    }
    case ProgramStage::kCompute:
    {
        ReplaceAll(text, kComputeDefinition, kMainDefinition);
    }
    default:
        break;
    }
}

// Shader cache
ShaderCache::ShaderCache(Device* device)
    : m_Device(device)
{
}

CompiledProgram& ShaderCache::Compile(const std::string& name, const std::string& source)
{
    auto hash = Hash<uint64_t>(source);
    if (m_Programs.contains(hash))
    {
        return *m_Programs[hash];
    }

    auto& program = *(m_Programs[hash] = std::make_unique<CompiledProgram>());
    PopulateProgram(name, source, program);
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

bool ShaderCache::PopulateProgram(const std::string& name, const std::string& source, CompiledProgram& compiled)
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

    auto addShader = [&](std::string_view text, EShLanguage lang, VkShaderStageFlagBits bit, const char* entry)
    {
        if (text.empty()) return;

        auto& shader = *shaders.emplace_back(std::make_unique<glslang::TShader>(lang));
        auto data = text.data();
        auto names = name.c_str();
        int length = (int)text.length();

        shader.setStringsWithLengthsAndNames(&data, &length, &names, 1);
        shader.setEnvInput(inputLang, lang, client, defaultVersion);
        shader.setEnvClient(client, clientVersion);
        shader.setEnvTarget(targetLang, targetLangVersion);

        auto result = shader.parse(&builtInResource, defaultVersion, true, EShMsgDefault);
        LC_ASSERT(result);

        program.addShader(&shader);

        auto stage = compiled.numStages;
        compiled.stages[stage].stageBit = bit;
        compiled.numStages++;
    };

    // Detect shader types in source
    if (source.find(kVertexDefinition) != std::string::npos &&
        source.find(kFragmentDefinition) != std::string::npos)
    {
        std::string vert(source);
        StripShader(vert, ProgramStage::kVertex);

        std::string frag(source);
        StripShader(frag, ProgramStage::kFragment);

        addShader(vert, EShLangVertex, VK_SHADER_STAGE_VERTEX_BIT, "vert");
        addShader(frag, EShLangFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "frag");
    }
    else if (source.find(kComputeDefinition) != std::string::npos)
    {
        std::string compute(source);
        addShader(compute, EShLangCompute, VK_SHADER_STAGE_COMPUTE_BIT, "compute");
    }
    else
    {
        LC_ERROR("No entrypoint found in {}", name);
        return false;
    }


    auto result = program.link(EShMsgDefault);
    LC_ASSERT(result);

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

        result = vkCreateShaderModule(m_Device->m_Device, &moduleInfo, nullptr, &compiled.stages[i].module);
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

        // TODO: Check errors here
        vkDestroyPipelineLayout(m_Device->m_Device, program->layout, nullptr);

        for (int i = 0; i < program->numSets; ++i)
        {
            vkDestroyDescriptorSetLayout(m_Device->m_Device, program->setLayouts[i], nullptr);
        }
    }
}

}
