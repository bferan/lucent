#include "Context.hpp"

#include "core/Hash.hpp"

namespace lucent
{

Context::Context(Device& device)
    : m_Device(device)
{
    // Create command pool
    auto cmdPoolCreateInfo = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = device.m_GraphicsQueue.familyIndex
    };

    auto result = vkCreateCommandPool(device.m_Handle, &cmdPoolCreateInfo, nullptr, &m_CommandPool);
    LC_ASSERT(result == VK_SUCCESS);

    auto bufferAllocInfo = VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1u
    };

    result = vkAllocateCommandBuffers(device.m_Handle, &bufferAllocInfo, &m_CommandBuffer);
    LC_ASSERT(result == VK_SUCCESS);

    // Create descriptor pool
    VkDescriptorPoolSize descPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1024
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1024
        }
    };

    auto descPoolInfo = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1024,
        .poolSizeCount = LC_ARRAY_SIZE(descPoolSizes),
        .pPoolSizes = descPoolSizes
    };

    result = vkCreateDescriptorPool(device.m_Handle, &descPoolInfo, nullptr, &m_DescriptorPool);
    LC_ASSERT(result == VK_SUCCESS);
}

void Context::Begin() const
{
    vkResetCommandPool(m_Device.m_Handle, m_CommandPool, 0);

    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    auto result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    LC_ASSERT(result == VK_SUCCESS);
}

void Context::End() const
{
    auto result = vkEndCommandBuffer(m_CommandBuffer);
    LC_ASSERT(result == VK_SUCCESS);
}

void Context::BeginRenderPass(const Framebuffer& fbuffer, VkExtent2D extent)
{
    if (fbuffer.usage == FramebufferUsage::SwapchainImage)
        m_SwapchainWritten = true;

    if (extent.width == 0 | extent.height == 0)
        extent = fbuffer.extent;

    auto viewport = VkViewport{
        .x = 0.0f, .y = static_cast<float>(extent.height),
        .width = static_cast<float>(extent.width),
        .height = -static_cast<float>(extent.height),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);

    VkClearValue clearValues[] = {
        { .color = { 0.0f, 0.0f, 0.0f, 1.0f }},
        { .depthStencil = { .depth = 1.0f, .stencil = 0 }}
    };

    auto renderPassBeginInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = fbuffer.renderPass,
        .framebuffer = fbuffer.handle,
        .renderArea = {
            .offset = {},
            .extent = fbuffer.extent
        },
        .clearValueCount = LC_ARRAY_SIZE(clearValues),
        .pClearValues = clearValues
    };
    vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Context::EndRenderPass() const
{
    vkCmdEndRenderPass(m_CommandBuffer);
}

void Context::Bind(const Pipeline* pipeline)
{
    vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle);
    m_BoundPipeline = pipeline;
}

void Context::Bind(const Buffer* buffer)
{
    switch (buffer->type)
    {
    case BufferType::Vertex:
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &buffer->handle, &offset);
        break;
    }
    case BufferType::Index:
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, buffer->handle, 0, VK_INDEX_TYPE_UINT32);
        break;
    }
    default:
    {
        LC_ERROR("Attempted to bind invalid buffer type with Context::Bind");
        LC_ASSERT(0);
        break;
    }
    }
}

void Context::Draw(uint32 indexCount)
{
    EstablishBindings();
    vkCmdDrawIndexed(m_CommandBuffer, indexCount, 1, 0, 0, 0);
}

void Context::CopyTexture(
    Texture* src, int srcLayer, int srcLevel,
    Texture* dst, int dstLayer, int dstLevel,
    uint32 width, uint32 height)
{
    auto transition = [&](VkImage img,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkAccessFlags srcAccess,
        VkPipelineStageFlags dstStage, VkAccessFlags dstAccess,
        uint32 layer, uint32 level)
    {
        auto imgBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = srcAccess,
            .dstAccessMask = dstAccess,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = img,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = level,
                .levelCount = 1,
                .baseArrayLayer = layer,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(m_CommandBuffer, srcStage, dstStage,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imgBarrier);
    };

    // SRC: COLOR_ATTACHMENT -> TRANSFER_SRC
    transition(src->image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, srcLayer, srcLevel);

    // DST: UNKNOWN -> TRANSFER_DST
    transition(dst->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_ACCESS_NONE_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, dstLayer, dstLevel);

    auto copyInfo = VkImageCopy{
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = static_cast<uint32>(srcLevel),
            .baseArrayLayer = static_cast<uint32>(srcLayer),
            .layerCount = 1
        },
        .srcOffset = {},
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = static_cast<uint32>(dstLevel),
            .baseArrayLayer = static_cast<uint32>(dstLayer),
            .layerCount = 1
        },
        .dstOffset = {},
        .extent = {
            .width = width,
            .height = height,
            .depth = 1
        }
    };
    vkCmdCopyImage(m_CommandBuffer,
        src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyInfo
    );

    // SRC: TRANSFER_SRC -> COLOR_ATTACHMENT
    transition(src->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, srcLayer, srcLevel);

    // DST: TRANSFER_DST -> SHADER_READ
    transition(dst->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_SHADER_READ_BIT, dstLayer, dstLevel);
}

void Context::Dispose()
{
    vkFreeCommandBuffers(m_Device.m_Handle, m_CommandPool, 1, &m_CommandBuffer);
    vkDestroyCommandPool(m_Device.m_Handle, m_CommandPool, nullptr);

    vkDestroyDescriptorPool(m_Device.m_Handle, m_DescriptorPool, nullptr);
}

void Context::Bind(uint32 set, uint32 binding, const Buffer* buffer)
{
    LC_ASSERT(buffer->type == BufferType::Uniform);

    auto& bound = m_BoundSets[set];
    bound.bindings[binding] = buffer;
    bound.dirty = true;
}

void Context::Bind(uint32 set, uint32 binding, const Buffer* buffer, uint32 dynamicOffset)
{
    LC_ASSERT(buffer->type == BufferType::UniformDynamic);

    auto& bound = m_BoundSets[set];
    bound.bindings[binding] = buffer;
    // TODO: Ensure order of dynamic offsets matches order in descriptor set
    bound.dynamicOffsets.push_back(dynamicOffset);
    bound.dirty = true;
}

void Context::Bind(uint32 set, uint32 binding, const Texture* texture)
{
    auto& bound = m_BoundSets[set];
    bound.bindings[binding] = texture;
    bound.dirty = true;
}

void Context::EstablishBindings()
{
    LC_ASSERT(m_BoundPipeline != nullptr);

    for (int setIndex = 0; setIndex < m_BoundSets.size(); ++setIndex)
    {
        auto& set = m_BoundSets[setIndex];
        if (set.dirty)
        {
            // Find/alloc descriptor set
            auto layout = m_BoundPipeline->shader->setLayouts[setIndex];
            auto setHandle = FindDescriptorSet(set.bindings, layout);

            // Bind set
            vkCmdBindDescriptorSets(m_CommandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_BoundPipeline->shader->pipelineLayout,
                setIndex,
                1,
                &setHandle,
                set.dynamicOffsets.size(),
                set.dynamicOffsets.data());

            set = {};
        }
    }
}

size_t Context::BindingHash::operator()(const Context::BindingArray& bindings) const
{
    return HashBytes<size_t>(bindings);
}

VkDescriptorSet Context::FindDescriptorSet(const BindingArray& bindings, VkDescriptorSetLayout layout)
{
    auto it = m_DescriptorSets.find(bindings);
    if (it == m_DescriptorSets.end())
    {
        // Allocate new set
        auto allocInfo = VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_DescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &layout,
        };

        VkDescriptorSet set;
        auto result = vkAllocateDescriptorSets(m_Device.m_Handle, &allocInfo, &set);
        LC_ASSERT(result == VK_SUCCESS);

        // Fill set with bindings
        Array <VkWriteDescriptorSet, Shader::kMaxBindingsPerSet> writes{};
        Array <VkDescriptorBufferInfo, Shader::kMaxBindingsPerSet> bufferWrites{};
        Array <VkDescriptorImageInfo, Shader::kMaxBindingsPerSet> imageWrites{};

        for (int bindIndex = 0; bindIndex < bindings.size(); ++bindIndex)
        {
            auto& binding = bindings[bindIndex];

            VkDescriptorImageInfo* imageInfo = nullptr;
            VkDescriptorBufferInfo* bufferInfo = nullptr;
            VkDescriptorType descriptorType{};

            if (holds_alternative<NullBinding>(binding))
            {
                continue;
            }
            else if (std::holds_alternative<BufferBinding>(binding))
            {
                auto buffer = get<BufferBinding>(binding);
                bufferInfo = &bufferWrites.emplace_back(VkDescriptorBufferInfo{
                    .buffer = buffer->handle,
                    .offset = 0,
                    .range = VK_WHOLE_SIZE
                });

                switch (buffer->type)
                {
                case BufferType::Uniform:
                    descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
                case BufferType::UniformDynamic:
                    descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    break;
                default:
                    LC_ERROR("Attempted to bind non-uniform buffer as descriptor");
                    LC_ASSERT(0);
                }
            }
            else if (std::holds_alternative<TextureBinding>(binding))
            {
                auto texture = get<TextureBinding>(binding);
                imageInfo = &imageWrites.emplace_back(VkDescriptorImageInfo{
                    .sampler = texture->sampler,
                    .imageView = texture->imageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
                descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            }

            writes.emplace_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = static_cast<uint32>(bindIndex),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = descriptorType,
                .pImageInfo = imageInfo,
                .pBufferInfo = bufferInfo
            });
        }
        vkUpdateDescriptorSets(m_Device.m_Handle, writes.size(), writes.data(), 0, nullptr);

        it = m_DescriptorSets.insert(it, { bindings, set });
    }
    return it->second;
}

}
