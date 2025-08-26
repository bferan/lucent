#include "ShaderCache.hpp"

#include "glslang/Public/ShaderLang.h"
#include "glslang/Include/Types.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include "core/Utility.hpp"
#include "device/vulkan/VulkanDevice.hpp"
#include "ResourceLimits.hpp"

namespace lucent
{

// Shader descriptor layout
static VkDescriptorType TypeToDescriptorType(const glslang::TType& type)
{
    switch (type.getBasicType())
    {
    case glslang::EbtBlock:
        return type.getQualifier().storage == glslang::EvqBuffer ?
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    case glslang::EbtSampler:
        return type.getSampler().image ?
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    default:
        LC_ASSERT(0 && "Unsupported uniform type declared in shader");
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

// Shader stripping
constexpr auto kVertexDefinition = "void Vertex()";
constexpr auto kFragmentDefinition = "void Fragment()";
constexpr auto kComputeDefinition = "void Compute()";
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
    // TODO: Make this more robust to comments?
    size_t pos = 0;
    while ((pos = text.find(keyword, pos)) != std::string::npos)
    {
        // Ensure we're not inside a line comment
        if (auto start = text.rfind("//", pos); start != std::string::npos)
        {
            if (auto end = text.find('\n', start); end != std::string::npos && end > pos)
            {
                pos = end;
                continue;
            }
        }

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
    // TODO: This could be performed in one pass per shader rather than separate string manipulations
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

// Shader including & resolution
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
        }
        else
        {
            // Set root path to "shaders" subdirectory in current working directory
            m_RootPath = "shaders";
            LC_INFO("Using shaders subdirectory for shader resolution: {}", m_RootPath);
        }

        // Ensure path ends with separator
        if (!m_RootPath.empty() && !m_RootPath.ends_with(kPathSeparator))
        {
            m_RootPath += kPathSeparator;
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
    {}

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
ShaderCache::ShaderCache(VulkanDevice* device)
    : m_Device(device)
    , m_Resolver(new DefaultResolver())
{}

VulkanShader* ShaderCache::Compile(const PipelineSettings& settings)
{
    ShaderInfoLog log{};
    auto result = Compile(settings, log);
    if (!result)
    {
        LC_ERROR("Error compiling shader {}:\n{}", settings.shaderName, log.error);
    }
    return result;
}

VulkanShader* ShaderCache::Compile(const PipelineSettings& settings, ShaderInfoLog& log)
{
    auto result = m_Resolver->Resolve(settings.shaderName);
    if (!result.found)
    {
        log.Error("Unable to resolve shader with name:");
        log.Error(settings.shaderName);
        return nullptr;
    }

    auto hash = Hash<uint64>(result.source);
    for (auto define: settings.shaderDefines)
        hash ^= Hash<uint64>(define);

    if (m_Shaders.contains(hash))
    {
        return m_Shaders[hash].get();
    }

    auto& shader = (m_Shaders[hash] = std::make_unique<VulkanShader>(VulkanShader{}));
    if (!PopulateShaderModules(*shader, result.qualifiedName, result.source, settings.shaderDefines, log))
    {
        m_Shaders.erase(hash);
        return nullptr;
    }
    shader->hash = hash;

    return shader.get();
}

void ShaderCache::Release(VulkanShader* shader)
{
    LC_ASSERT(m_Shaders.contains(shader->hash));

    if (--shader->uses <= 0)
    {
        FreeResources(shader);
        m_Shaders.erase(shader->hash);
    }
}

const char* kDefaultPreamble =
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_GOOGLE_include_directive : enable\n";

static bool ScanLinkerSymbols(const TIntermNode& root,
    ShaderCache::PipelineLayout& layout,
    VulkanShader& shader,
    ShaderInfoLog& log)
{
    auto addDescriptor = [&](glslang::TIntermSymbol& symbol)
    {
        auto& type = symbol.getType();
        auto& qualifier = type.getQualifier();
        auto set = qualifier.layoutSet;
        auto binding = qualifier.layoutBinding;

        // Validate binding
        if (set < 0 || set >= VulkanShader::kMaxSets ||
            binding < 0 || binding >= VulkanShader::kMaxBindingsPerSet)
        {
            log.Error("Error while scanning: invalid uniform set/binding in shader");
            return false;
        }

        if (!layout.sets[set])
            layout.sets[set] = ShaderCache::SetLayout{};

        // If binding not already present from a previous stage, create it
        if (!layout.sets[set]->bindings[binding])
        {
            layout.sets[set]->bindings[binding] = TypeToDescriptorType(type);

            bool isBlock = type.getBasicType() == glslang::EbtBlock;
            uint32 size = isBlock ? glslang::TIntermediate::getBlockSize(type) : 0;

            // Add global scope descriptor
            auto entry = Descriptor{
                .hash = Hash<uint32>(symbol.getAccessName()),
                .set = set,
                .binding = binding,
                .size = size
            };
            shader.descriptors.emplace_back(entry);

            // Add all block members as separate descriptors
            if (isBlock)
            {
                shader.blocks.emplace_back(entry);
                int childSize = 0;
                int offset = 0;
                for (auto& loc: *type.getStruct())
                {
                    auto& child = *loc.type;
                    glslang::TIntermediate::updateOffset(type, child, offset, childSize);

                    shader.descriptors.emplace_back(Descriptor{
                        .hash = Hash<uint32>(child.getFieldName()),
                        .set = set,
                        .binding = binding,
                        .offset = static_cast<uint32>(offset),
                        .size = static_cast<uint32>(childSize)
                    });
                    offset += childSize;
                }
            }
        }
        return true;
    };

    // Scan all uniform linker symbols
    for (auto node: root.getAsAggregate()->getSequence())
    {
        auto agg = node->getAsAggregate();
        if (agg->getOp() == glslang::EOpLinkerObjects)
        {
            for (auto obj: agg->getSequence())
            {
                auto symbol = obj->getAsSymbolNode();
                if (symbol->getQualifier().hasUniformLayout())
                {
                    if (!addDescriptor(*symbol))
                        return false;
                }
            }
        }
    }
    return true;
}

bool ShaderCache::PopulateShaderModules(VulkanShader& shader,
    const std::string& name,
    const std::string& source,
    const std::vector<std::string_view>& defines,
    ShaderInfoLog& log)
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

    // Construct common preamble string
    auto preamble = std::string(kDefaultPreamble);
    for (auto& define: defines)
    {
        preamble += "#define ";
        preamble += define;
        preamble += "\n";
    }

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
        glsl.setPreamble(preamble.c_str());
        glsl.setAutoMapBindings(true);

        Includer includer(m_Resolver.get(), shaderStage);

        auto result = glsl.parse(&builtInResource, defaultVersion, true, EShMsgDefault, includer);
        if (!result)
        {
            log.Error("Failed parsing:");
            log.Error(glsl.getInfoLog());
            return false;
        }

        program.addShader(&glsl);
        shader.stages.emplace_back(VulkanShader::Stage{ .stageBit = bit });
        return true;
    };

    // Detect shader types in source
    auto foundVertex = source.find(kVertexDefinition) != std::string::npos;
    auto foundFragment = source.find(kFragmentDefinition) != std::string::npos;
    if (foundVertex || foundFragment)
    {
        if (foundVertex)
        {
            std::string vert(source);
            StripShader(vert, ShaderStage::kVertex);

            if (!addShader(vert, EShLangVertex, VK_SHADER_STAGE_VERTEX_BIT,
                "vert", ShaderStage::kVertex))
                return false;
        }
        if (foundFragment)
        {
            std::string frag(source);
            StripShader(frag, ShaderStage::kFragment);

            if (!addShader(frag, EShLangFragment, VK_SHADER_STAGE_FRAGMENT_BIT,
                "frag", ShaderStage::kFragment))
                return false;
        }
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
        log.Error("No suitable entrypoint combination found in shader.");
        return false;
    }

    if (!program.link(EShMsgDefault))
    {
        log.Error("Failed linking:");
        log.Error(program.getInfoLog());
        return false;
    }

    PipelineLayout layout;

    // Create SPIRV shader modules for each stage
    for (int i = 0; i < shader.stages.size(); ++i)
    {
        m_SpirvBuffer.clear();

        auto& inter = *shaders[i]->getIntermediate();
        glslang::GlslangToSpv(inter, m_SpirvBuffer);

        auto moduleInfo = VkShaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = m_SpirvBuffer.size() * sizeof(uint32),
            .pCode = m_SpirvBuffer.data()
        };

        auto result = vkCreateShaderModule(m_Device->GetHandle(), &moduleInfo, nullptr, &shader.stages[i].module);
        if (result != VK_SUCCESS)
        {
            log.Error("Failed to create Vulkan shader module");
            return false;
        }

        if (!ScanLinkerSymbols(*inter.getTreeRoot(), layout, shader, log))
            return false;
    }

    // Sort descriptors in hash order for binary search
    auto& descriptors = shader.descriptors;
    std::sort(descriptors.begin(), descriptors.end(), [](auto& a, auto& b)
    {
        return a.hash < b.hash;
    });

    // Check for any hash collisions
    uint32 prevHash = -1;
    for (auto entry: descriptors)
    {
        uint32 hash = entry.hash;
        if (hash == prevHash)
        {
            log.Error("Descriptor layout hash collision occurred");
            return false;
        }
        prevHash = hash;
    }

    return PopulateShaderLayout(shader, layout, log);
}

bool ShaderCache::PopulateShaderLayout(VulkanShader& shader, const PipelineLayout& layout, ShaderInfoLog& log)
{
    // Find or create descriptor sets
    for (int i = 0; i < VulkanShader::kMaxSets; ++i)
    {
        auto& set = layout.sets[i];
        if (!set) break;
        shader.setLayouts.push_back(FindSetLayout(set.value()));
    }

    // Find or create pipeline layout
    auto it = m_PipelineLayouts.find(shader.setLayouts);
    if (it == m_PipelineLayouts.end())
    {
        auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32>(shader.setLayouts.size()),
            .pSetLayouts = shader.setLayouts.data()
        };

        VkPipelineLayout pipelineLayout;
        auto result = vkCreatePipelineLayout(m_Device->GetHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
        LC_ASSERT(result == VK_SUCCESS);

        it = m_PipelineLayouts.insert(it, { shader.setLayouts, pipelineLayout });
    }

    shader.pipelineLayout = it->second;
    return true;
}

void ShaderCache::Clear()
{
    for (auto&[hash, shader]: m_Shaders)
    {
        FreeResources(shader.get());
    }

    for (auto&[layout, vkLayout]: m_SetLayouts)
    {
        vkDestroyDescriptorSetLayout(m_Device->GetHandle(), vkLayout, nullptr);
    }

    for (auto&[layout, vkLayout]: m_PipelineLayouts)
    {
        vkDestroyPipelineLayout(m_Device->GetHandle(), vkLayout, nullptr);
    }
}

void ShaderCache::FreeResources(VulkanShader* shader)
{
    for (auto& stage: shader->stages)
    {
        vkDestroyShaderModule(m_Device->GetHandle(), stage.module, nullptr);
    }
}

VkDescriptorSetLayout ShaderCache::FindSetLayout(const SetLayout& layout)
{
    auto it = m_SetLayouts.find(layout);

    if (it != m_SetLayouts.end())
        return it->second;

    // Create new descriptor set layout
    uint32 numBindings = 0;
    VkDescriptorSetLayoutBinding bindings[VulkanShader::kMaxBindingsPerSet];
    for (uint32 binding = 0; binding < layout.bindings.size(); ++binding)
    {
        if (layout.bindings[binding])
        {
            bindings[numBindings++] = VkDescriptorSetLayoutBinding{
                .binding = binding,
                .descriptorType = layout.bindings[binding].value(),
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_ALL
            };
        }
    }

    auto setLayoutInfo = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = numBindings,
        .pBindings = bindings
    };

    VkDescriptorSetLayout setLayout;
    auto result = vkCreateDescriptorSetLayout(m_Device->GetHandle(),
        &setLayoutInfo,
        nullptr,
        &setLayout);

    LC_ASSERT(result == VK_SUCCESS);

    m_SetLayouts.insert(it, { layout, setLayout });
    return setLayout;
}

size_t ShaderCache::LayoutHash::operator()(const SetLayout& set) const
{
    size_t hash = FNVTraits<size_t>::kOffset;
    for (int i = 0; i < set.bindings.size(); ++i)
    {
        hash = HashBytes(i, hash);
        if (set.bindings[i])
            hash = HashBytes(set.bindings[i].value(), hash);
    }
    return hash;
}

size_t ShaderCache::LayoutHash::operator()(const SetList& sets) const
{
    return HashBytes<size_t>(sets);
}

}
