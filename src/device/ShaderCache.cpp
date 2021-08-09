#include "device/ShaderCache.hpp"

#include "glslang/Public/ShaderLang.h"
#include "glslang/Include/Types.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include "core/Utility.hpp"
#include "core/Hash.hpp"
#include "device/ResourceLimits.hpp"

namespace lucent
{

size_t LayoutHash::operator()(const SetLayout& set) const
{
    return 0;
}
size_t LayoutHash::operator()(const PipelineLayout& layout) const
{
    return 0;
}

// Shader descriptor layout
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

static void StripShader(std::string& text, ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::kVertex:
    {
        StripFunction(text, kFragmentDefinition);
        StripDeclarations(text, "out ");
        ReplaceAll(text, "attribute ", "in ");
        ReplaceAll(text, "varying ", "out ");
        ReplaceAll(text, kVertexDefinition, kMainDefinition);
        break;
    }
    case ShaderStage::kFragment:
    {
        StripFunction(text, kVertexDefinition);
        StripDeclarations(text, "attribute ");
        ReplaceAll(text, "varying ", "in ");
        ReplaceAll(text, kFragmentDefinition, kMainDefinition);
        break;
    }
    case ShaderStage::kCompute:
    {
        ReplaceAll(text, kComputeDefinition, kMainDefinition);
    }
    default:
        break;
    }
}

// Shader including/resolution
class DefaultResolver : public ShaderResolver
    {
    public:
        DefaultResolver()
        {
            auto env = std::getenv(kShaderEnvVar);
            if (env)
            {
                LC_DEBUG("Resolving shaders from {}", env);
                m_RootPath = env;
                if (!m_RootPath.empty() && !m_RootPath.ends_with(kPathSeparator))
                {
                    m_RootPath += kPathSeparator;
                }
            }
            else
            {
                LC_INFO("Default shader resolver could not locate ENV variable {}\n"
                        "Using current working directory instead.", kShaderEnvVar);
            }
        }

        Result Resolve(const std::string& name) override
        {
            Result result{};
            result.qualifiedName = m_RootPath + name;

            if (!result.qualifiedName.ends_with(kShaderExt))
                result.qualifiedName += kShaderExt;

            result.source = ReadFile(result.qualifiedName, std::ios::in, &result.found);
            return result;
        }

        ~DefaultResolver() override = default;

    private:
        static constexpr auto kShaderEnvVar = "LC_SHADER_ROOT";
        static constexpr char kPathSeparator = '/';
        static constexpr const char* kShaderExt = ".shader";

        std::string m_RootPath{};

    };

class Includer : public glslang::TShader::Includer
    {
    public:
        Includer(ShaderResolver* resolver, ShaderStage stage)
        : m_Resolver(resolver), m_Stage(stage)
        {
        }

        // glslang callbacks
        IncludeResult* includeSystem(const char* header, const char* includer, size_t depth) override
        {
            return nullptr;
        }

        IncludeResult* includeLocal(const char* header, const char* includer, size_t depth) override
        {
            auto result = m_Resolver->Resolve(header);

            if (!result.found)
                return nullptr;

            auto text = new std::string(std::move(result.source));
            StripShader(*text, m_Stage);

            return new IncludeResult(result.qualifiedName, text->data(), text->size(), text);
        }

        void releaseInclude(IncludeResult* result) override
        {
            if (result)
            {
                delete (std::string*)result->userData;
                delete result;
            }
        }

    private:
        ShaderResolver* m_Resolver;
        ShaderStage m_Stage;
    };

// Shader cache
ShaderCache::ShaderCache(Device* device)
    : m_Device(device)
    , m_Resolver(new DefaultResolver())
{}

Shader* ShaderCache::Compile(const std::string& name)
{
    ShaderInfoLog log{};
    auto result = Compile(name, log);
    if (!result)
    {
        LC_ERROR("Error compiling shader {}:\n{}", name, log.error);
    }
    return result;
}

Shader* ShaderCache::Compile(const std::string& name, ShaderInfoLog& log)
{
    auto result = m_Resolver->Resolve(name);
    if (!result.found)
    {
        log.Error("Unable to resolve shader with name:");
        log.Error(name);
        return nullptr;
    }

    auto hash = Hash<uint64_t>(result.source);
    if (m_Shaders.contains(hash))
    {
        return m_Shaders[hash].get();
    }

    auto& shader = (m_Shaders[hash] = std::make_unique<Shader>());
    if (!PopulateShader(result.qualifiedName, result.source, *shader, log))
    {
        m_Shaders.erase(hash);
        return nullptr;
    }
    shader->hash = hash;

    return shader.get();
}

void ShaderCache::Release(Shader* shader)
{
    LC_ASSERT(m_Shaders.contains(shader->hash));

    if (--shader->uses <= 0)
    {
        FreeResources(shader);
        m_Shaders.erase(shader->hash);
    }
}

static bool ScanLayout(const TIntermNode& root, PipelineLayout& layout, ShaderInfoLog& log)
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

                    if (set >= 0 && set < Shader::kMaxSets &&
                        binding >= 0 && binding < Shader::kMaxBindingsPerSet)
                    {
                        if (!layout.sets[set])
                            layout.sets[set] = SetLayout{};

                        auto& setLayout = layout.sets[set].value();
                        setLayout.bindings[binding] = BasicTypeToDescriptorType(type.getBasicType());
                    }
                    else
                    {
                        log.Error("Error while scanning: Invalid uniform binding in shader");
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool ShaderCache::PopulateShader(const std::string& name, const std::string& source, Shader& shader, ShaderInfoLog& log)
{
    const int defaultVersion = 450;
    const auto inputLang = glslang::EShSourceGlsl;
    const auto client = glslang::EShClientVulkan;
    const auto clientVersion = glslang::EShTargetVulkan_1_2;
    const auto targetLang = glslang::EshTargetSpv;
    const auto targetLangVersion = glslang::EShTargetSpv_1_2;

    shader = {};
    std::vector<std::unique_ptr<glslang::TShader>> shaders;
    glslang::TProgram program;

    auto addShader = [&](std::string_view text,
        EShLanguage lang,
        VkShaderStageFlagBits bit,
        const char* entry,
        ShaderStage shaderStage)
    {
        if (text.empty())
        {
            log.Error("Empty shader stage");
            return false;
        }

        auto& glsl = *shaders.emplace_back(std::make_unique<glslang::TShader>(lang));
        auto data = text.data();
        auto names = name.c_str();
        int length = (int)text.length();

        glsl.setStringsWithLengthsAndNames(&data, &length, &names, 1);
        glsl.setEnvInput(inputLang, lang, client, defaultVersion);
        glsl.setEnvClient(client, clientVersion);
        glsl.setEnvTarget(targetLang, targetLangVersion);

        Includer includer(m_Resolver.get(), shaderStage);

        auto result = glsl.parse(&builtInResource, defaultVersion, true, EShMsgDefault, includer);
        if (!result)
        {
            log.Error("Failed parsing:");
            log.Error(glsl.getInfoLog());
            return false;
        }

        program.addShader(&glsl);

        auto stage = shader.numStages;
        shader.stages[stage].stageBit = bit;
        shader.numStages++;

        return true;
    };

    // Detect shader types in source
    if (source.find(kVertexDefinition) != std::string::npos &&
        source.find(kFragmentDefinition) != std::string::npos)
    {
        std::string vert(source);
        StripShader(vert, ShaderStage::kVertex);

        std::string frag(source);
        StripShader(frag, ShaderStage::kFragment);

        if (!addShader(vert, EShLangVertex, VK_SHADER_STAGE_VERTEX_BIT,
            "vert", ShaderStage::kVertex))
            return false;
        if (!addShader(frag, EShLangFragment, VK_SHADER_STAGE_FRAGMENT_BIT,
            "frag", ShaderStage::kFragment))
            return false;
    }
    else if (source.find(kComputeDefinition) != std::string::npos)
    {
        std::string compute(source);
        StripShader(compute, ShaderStage::kCompute);

        if (!addShader(compute, EShLangCompute, VK_SHADER_STAGE_COMPUTE_BIT,
            "compute", ShaderStage::kCompute))
            return false;
    }
    else
    {
        log.Error("No entrypoint found in shader.");
        return false;
    }

    if (!program.link(EShMsgDefault))
    {
        log.Error("Failed linking:");
        log.Error(program.getInfoLog());
        return false;
    }

    // Create SPIRV shader modules for each stage
    PipelineLayout layout;
    for (int i = 0; i < shader.numStages; ++i)
    {
        m_SpirvBuffer.clear();

        auto& inter = *shaders[i]->getIntermediate();
        glslang::GlslangToSpv(inter, m_SpirvBuffer);

        auto moduleInfo = VkShaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = m_SpirvBuffer.size() * sizeof(uint32_t),
            .pCode = m_SpirvBuffer.data()
        };

        auto result = vkCreateShaderModule(m_Device->m_Device, &moduleInfo, nullptr, &shader.stages[i].module);
        if (result != VK_SUCCESS)
        {
            log.Error("Failed to create Vulkan shader module");
            return false;
        }

        if (!ScanLayout(*inter.getTreeRoot(), layout, log))
            return false;
    }

    return PopulateLayout(layout, shader, log);
}

bool ShaderCache::PopulateLayout(const PipelineLayout& layout, Shader& compiled, ShaderInfoLog& log)
{
    // Create descriptor set layout per set
    for (const auto& set : layout.sets)
    {
        if (set)
        {
            auto& setLayout = set.value();

            uint32_t numBindings = 0;
            VkDescriptorSetLayoutBinding bindings[Shader::kMaxBindingsPerSet];
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

    return true;
}

void ShaderCache::Clear()
{
    for (auto&[hash, shader] : m_Shaders)
    {
        FreeResources(shader.get());
    }

    for (auto&[layout, vkLayout] : m_SetLayouts)
    {
        vkDestroyDescriptorSetLayout(m_Device->m_Device, vkLayout, nullptr);
    }

    for (auto&[layout, vkLayout] : m_PipelineLayouts)
    {
        vkDestroyPipelineLayout(m_Device->m_Device, vkLayout, nullptr);
    }
}

void ShaderCache::FreeResources(Shader* shader)
{
    for (int i = 0; i < shader->numStages; ++i)
    {
        vkDestroyShaderModule(m_Device->m_Device, shader->stages[i].module, nullptr);
    }
}

VkDescriptorSetLayout ShaderCache::FindSetLayout(const SetLayout& layout)
{
    return nullptr;
}

VkPipelineLayout ShaderCache::FindPipelineLayout(const PipelineLayout& layout)
{
    return nullptr;
}

}
