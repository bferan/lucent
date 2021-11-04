#include "VulkanPipeline.hpp"

#include "device/vulkan/VulkanFramebuffer.hpp"
#include "device/vulkan/VulkanDevice.hpp"
#include "rendering/Geometry.hpp"
#include "rendering/Mesh.hpp"

namespace lucent
{

using Vertex = Mesh::Vertex;

VulkanPipeline::VulkanPipeline(VulkanDevice* vulkanDevice, VulkanShader* vulkanShader, const PipelineSettings& info)
    : device(vulkanDevice)
    , shader(vulkanShader)
{
    m_Settings = info;

    LC_ASSERT(shader);
    shader->uses++;

    (info.type == PipelineType::kGraphics) ? InitGraphics() : InitCompute();
}

VulkanPipeline::~VulkanPipeline()
{
    if (handle)
    {
        vkDestroyPipeline(device->GetHandle(), handle, nullptr);
        shader->uses--;
    }
}

VulkanPipeline& VulkanPipeline::operator=(VulkanPipeline&& other) noexcept
{
    std::swap(m_Settings, other.m_Settings);
    std::swap(device, other.device);
    std::swap(shader, other.shader);
    std::swap(handle, other.handle);
    return *this;
}

void VulkanPipeline::InitGraphics()
{
    auto& settings = m_Settings;

    LC_ASSERT(settings.framebuffer);
    auto& framebuffer = *Get(settings.framebuffer);

    // Populate pipeline shader stages
    VkPipelineShaderStageCreateInfo stageInfos[VulkanShader::kMaxStages];
    for (int i = 0; i < shader->stages.size(); ++i)
    {
        auto& stage = shader->stages[i];
        stageInfos[i] = VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage.stageBit,
            .module = stage.module,
            .pName = "main"
        };
    }

    // Configure pipeline fixed functions
    VkVertexInputBindingDescription vertexBindingInfos[] = {
        {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    VkVertexInputAttributeDescription vertexAttributeInfos[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, normal)
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, tangent)
        },
        {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, texCoord0)
        },
        {
            .location = 4,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, color)
        }
    };

    auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = LC_ARRAY_SIZE(vertexBindingInfos),
        .pVertexBindingDescriptions = vertexBindingInfos,
        .vertexAttributeDescriptionCount = LC_ARRAY_SIZE(vertexAttributeInfos),
        .pVertexAttributeDescriptions = vertexAttributeInfos
    };

    auto inputAssemblyInfo = VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    auto viewport = VkViewport{
        .x = 0.0f, .y = 0.0f,
        .width = static_cast<float>(framebuffer.extent.width),
        .height = static_cast<float>(framebuffer.extent.height),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };

    auto scissor = VkRect2D{
        .offset = {},
        .extent = framebuffer.extent
    };

    auto viewportStateInfo = VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    auto rasterizationInfo = VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = static_cast<VkBool32>(settings.depthClampEnable),
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    auto multisampleInfo = VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = static_cast<VkSampleCountFlagBits>(framebuffer.samples),
        .sampleShadingEnable = VK_FALSE
    };

    auto depthStencilInfo = VkPipelineDepthStencilStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = static_cast<VkBool32>(settings.depthTestEnable ? VK_TRUE : VK_FALSE),
        .depthWriteEnable = static_cast<VkBool32>(settings.depthTestEnable ? VK_TRUE : VK_FALSE),
        .depthCompareOp = settings.depthTestEnable ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    Array <VkPipelineColorBlendAttachmentState, kMaxColorAttachments> colorAttachments;
    for (int i = 0; i < framebuffer.GetSettings().colorTextures.size(); ++i)
    {
        auto colorBlendAttachmentInfo = VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT
        };
        if (!settings.depthTestEnable)
        {
            colorBlendAttachmentInfo = VkPipelineColorBlendAttachmentState{
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                    | VK_COLOR_COMPONENT_A_BIT
            };
        }
        colorAttachments.push_back(colorBlendAttachmentInfo);
    }

    auto colorBlendInfo = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pAttachments = colorAttachments.data()
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT
    };

    auto dynamicInfo = VkPipelineDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = LC_ARRAY_SIZE(dynamicStates),
        .pDynamicStates = dynamicStates
    };

    auto pipelineCreateInfo = VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32>(shader->stages.size()),
        .pStages = stageInfos,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizationInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = &depthStencilInfo,
        .pColorBlendState = &colorBlendInfo,
        .pDynamicState = &dynamicInfo,
        .layout = shader->pipelineLayout,
        .renderPass = framebuffer.renderPass,
        .subpass = 0
    };
    LC_CHECK(vkCreateGraphicsPipelines(device->GetHandle(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &handle));
}

void VulkanPipeline::InitCompute()
{
    LC_ASSERT(shader->stages.size() == 1);
    auto& stage = shader->stages.back();

    auto computeInfo = VkComputePipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage.stageBit,
            .module = stage.module,
            .pName = "main"
        },
        .layout = shader->pipelineLayout
    };
    LC_CHECK(vkCreateComputePipelines(device->GetHandle(), VK_NULL_HANDLE, 1, &computeInfo, nullptr, &handle));
}

Descriptor* VulkanPipeline::Lookup(DescriptorID id) const
{
    auto& descriptors = shader->descriptors;

    auto it = std::lower_bound(descriptors.begin(), descriptors.end(), id,
        [](auto& desc, auto& name)
        {
            return desc.hash < name.hash;
        });

    if (it == descriptors.end() || id.hash != it->hash)
    {
        LC_ERROR("ERROR: Failed to find descriptor with ID \"{}\"", id.name);
        return nullptr;
    }
    return it;
}

}
