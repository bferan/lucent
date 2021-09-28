#include "VulkanContext.hpp"

#include "device/vulkan/VulkanBuffer.hpp"
#include "device/vulkan/VulkanFramebuffer.hpp"
#include "device/vulkan/VulkanPipeline.hpp"
#include "device/vulkan/VulkanTexture.hpp"

namespace lucent
{

static VkPipelineBindPoint GetBindPoint(PipelineType type)
{
    switch (type)
    {
    case PipelineType::kGraphics:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;

    case PipelineType::kCompute:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    }
}

VulkanContext::VulkanContext(VulkanDevice& device)
    : m_Device(device)
{
    // Create fence in active state
    auto fenceInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    LC_CHECK(vkCreateFence(device.m_Handle, &fenceInfo, nullptr, &m_ReadyFence));

    // Create command pool
    auto cmdPoolCreateInfo = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = device.m_GraphicsQueue.familyIndex
    };
    LC_CHECK(vkCreateCommandPool(device.m_Handle, &cmdPoolCreateInfo, nullptr, &m_CommandPool));

    auto bufferAllocInfo = VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    LC_CHECK(vkAllocateCommandBuffers(device.m_Handle, &bufferAllocInfo, &m_CommandBuffer));

    // Create descriptor pool
    VkDescriptorPoolSize descPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 4096
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 4096
        },
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 4096,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 4096
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 4096
        }
    };

    auto descPoolInfo = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 4096,
        .poolSizeCount = LC_ARRAY_SIZE(descPoolSizes),
        .pPoolSizes = descPoolSizes
    };
    LC_CHECK(vkCreateDescriptorPool(device.m_Handle, &descPoolInfo, nullptr, &m_DescriptorPool));
}

VulkanContext::~VulkanContext()
{
    vkFreeCommandBuffers(m_Device.m_Handle, m_CommandPool, 1, &m_CommandBuffer);
    vkDestroyCommandPool(m_Device.m_Handle, m_CommandPool, nullptr);

    vkDestroyFence(m_Device.m_Handle, m_ReadyFence, nullptr);
    vkDestroyDescriptorPool(m_Device.m_Handle, m_DescriptorPool, nullptr);
}

void VulkanContext::Begin()
{
    vkWaitForFences(m_Device.m_Handle, 1, &m_ReadyFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device.m_Handle, 1, &m_ReadyFence);

    vkResetCommandPool(m_Device.m_Handle, m_CommandPool, 0);

    vkResetDescriptorPool(m_Device.m_Handle, m_DescriptorPool, 0);
    m_DescriptorSets.clear();

    m_ScratchBindings.clear();
    m_ScratchDrawOffset = 0;

    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    LC_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
}

void VulkanContext::End()
{
    LC_CHECK(vkEndCommandBuffer(m_CommandBuffer));
}

void VulkanContext::BeginRenderPass(const Framebuffer* framebuffer)
{
    auto& fbuffer = *Get(framebuffer);
    m_BoundFramebuffer = &fbuffer;

    // Transition attachments
    for (auto color: framebuffer->colorTextures)
    {
        TransitionLayout(color, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    if (framebuffer->depthTexture)
    {
        TransitionLayout(framebuffer->depthTexture,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    Viewport(fbuffer.extent.width, fbuffer.extent.height);

    auto renderPassBeginInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = fbuffer.renderPass,
        .framebuffer = fbuffer.handle,
        .renderArea = {
            .offset = {},
            .extent = fbuffer.extent
        }
    };
    vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanContext::EndRenderPass()
{
    auto framebuffer = m_BoundFramebuffer;
    vkCmdEndRenderPass(m_CommandBuffer);

    // Transition attachments
    for (auto color: framebuffer->colorTextures)
    {
        RestoreLayout(color, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    if (framebuffer->depthTexture)
    {
        RestoreLayout(framebuffer->depthTexture,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
}

void VulkanContext::Clear(Color color, float depth)
{
    LC_ASSERT(m_BoundFramebuffer);

    Array <VkClearAttachment, kMaxAttachments> clears;

    auto rect = VkClearRect{
        .rect = { .offset = {}, .extent = m_BoundFramebuffer->extent },
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    for (uint32 attachment = 0; attachment < m_BoundFramebuffer->colorTextures.size(); ++attachment)
    {
        clears.push_back(VkClearAttachment{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .colorAttachment = attachment,
            .clearValue = VkClearValue{ .color = { color.r, color.g, color.b, color.a }}
        });
    }
    if (m_BoundFramebuffer->depthTexture)
    {
        clears.push_back(VkClearAttachment{
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue = VkClearValue{ .depthStencil = { .depth = depth }}
        });
    }

    vkCmdClearAttachments(m_CommandBuffer, clears.size(), clears.data(), 1, &rect);
}

void VulkanContext::Viewport(uint32 width, uint32 height)
{
    auto viewport = VkViewport{
        .x = 0.0f, .y = 0.0f,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
}

void VulkanContext::BindPipeline(const Pipeline* pipeline)
{
    m_BoundPipeline = Get(pipeline);
    vkCmdBindPipeline(m_CommandBuffer, GetBindPoint(pipeline->type), m_BoundPipeline->handle);
}

void VulkanContext::BindBuffer(const Buffer* buffer)
{
    auto& buff = *Get(buffer);
    switch (buff.type)
    {
    case BufferType::kVertex:
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &buff.handle, &offset);
        break;
    }
    case BufferType::kIndex:
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, buff.handle, 0, VK_INDEX_TYPE_UINT32);
        break;
    }
    default:
    {
        LC_ERROR("Attempted to bind invalid buffer type with VulkanContext::BindTexture");
        LC_ASSERT(0);
        break;
    }
    }
}

void VulkanContext::Draw(uint32 indexCount)
{
    EstablishBindings();
    vkCmdDrawIndexed(m_CommandBuffer, indexCount, 1, 0, 0, 0);
    ResetScratchBindings();
}

void VulkanContext::Dispatch(uint32 x, uint32 y, uint32 z)
{
    EstablishBindings();
    vkCmdDispatch(m_CommandBuffer, x, y, z);
    ResetScratchBindings();

    // TODO: Sync compute writes
}

void VulkanContext::CopyTexture(
    Texture* source, uint32 srcLayer, uint32 srcLevel,
    Texture* dest, uint32 dstLayer, uint32 dstLevel,
    uint32 width, uint32 height)
{
    auto src = Get(source);
    auto dst = Get(dest);

    TransitionLayout(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayer, srcLevel);
    TransitionLayout(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayer, dstLevel);

    auto copy = VkImageCopy{
        .srcSubresource = {
            .aspectMask = src->aspect,
            .mipLevel = srcLevel,
            .baseArrayLayer = srcLayer,
            .layerCount = 1
        },
        .srcOffset = {},
        .dstSubresource = {
            .aspectMask = dst->aspect,
            .mipLevel = dstLevel,
            .baseArrayLayer = dstLayer,
            .layerCount = 1
        },
        .dstOffset = {},
        .extent = { width, height, 1 }
    };
    vkCmdCopyImage(m_CommandBuffer, src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    RestoreLayout(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayer, srcLevel);
    RestoreLayout(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayer, dstLevel);
}

void VulkanContext::CopyTexture(
    Texture* source, uint32 srcLayer, uint32 srcLevel,
    Buffer* dest, uint32 offset,
    uint32 width, uint32 height)
{
    auto src = Get(source);
    auto dst = Get(dest);

    TransitionLayout(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayer, srcLevel);

    auto copy = VkBufferImageCopy{
        .bufferOffset = offset,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = src->aspect,
            .mipLevel = srcLevel,
            .baseArrayLayer = srcLayer,
            .layerCount = 1
        },
        .imageOffset = {},
        .imageExtent = { width, height, 1 }
    };
    vkCmdCopyImageToBuffer(m_CommandBuffer, src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->handle, 1, &copy);

    RestoreLayout(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayer, srcLevel);
}

void VulkanContext::CopyTexture(
    Buffer* source, uint32 offset,
    Texture* dest, uint32 dstLayer, uint32 dstLevel,
    uint32 width, uint32 height)
{
    auto src = Get(source);
    auto dst = Get(dest);

    TransitionLayout(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayer, dstLevel);

    auto copy = VkBufferImageCopy{
        .bufferOffset = offset,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = dst->aspect,
            .mipLevel = dstLevel,
            .baseArrayLayer = dstLayer,
            .layerCount = 1
        },
        .imageOffset = {},
        .imageExtent = { width, height, 1 }
    };
    vkCmdCopyBufferToImage(m_CommandBuffer, src->handle, dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    RestoreLayout(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayer, dstLevel);
}

void VulkanContext::BlitTexture(
    Texture* source, uint32 srcLayer, uint32 srcLevel,
    Texture* dest, uint32 dstLayer, uint32 dstLevel)
{
    auto src = Get(source);
    auto dst = Get(dest);

    auto srcWidth = Max((int32)src->width >> srcLevel, 1);
    auto srcHeight = Max((int32)src->height >> srcLevel, 1);

    auto dstWidth = Max((int32)dst->width >> dstLevel, 1);
    auto dstHeight = Max((int32)dst->height >> dstLevel, 1);

    TransitionLayout(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayer, srcLevel);
    TransitionLayout(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayer, dstLevel);

    auto blit = VkImageBlit{
        .srcSubresource = {
            .aspectMask = src->aspect,
            .mipLevel = srcLevel,
            .baseArrayLayer = srcLayer,
            .layerCount = 1
        },
        .srcOffsets = {{}, { srcWidth, srcHeight, 1 }},
        .dstSubresource = {
            .aspectMask = dst->aspect,
            .mipLevel = dstLevel,
            .baseArrayLayer = dstLayer,
            .layerCount = 1
        },
        .dstOffsets = {{}, { dstWidth, dstHeight, 1 }},
    };
    vkCmdBlitImage(m_CommandBuffer, src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    RestoreLayout(src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayer, srcLevel);
    RestoreLayout(dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayer, dstLevel);
}

void VulkanContext::GenerateMips(Texture* texture)
{
    for(int layer = 0; layer < texture->settings.layers; ++layer)
    {
        for (int level = 0; level < texture->settings.levels - 1; ++level)
        {
            BlitTexture(texture, layer, level, texture, layer, level + 1);
        }
    }
}

void VulkanContext::BindBuffer(Descriptor* descriptor, const Buffer* generalBuffer)
{
    auto buffer = Get(generalBuffer);
    LC_ASSERT(buffer->type == BufferType::kUniform || buffer->type == BufferType::kStorage);

    auto& bound = m_BoundSets[descriptor->set];
    bound.bindings[descriptor->binding] = { (buffer->type == BufferType::kUniform) ?
        Binding::kUniformBuffer : Binding::kStorageBuffer, buffer };
    bound.dirty = true;
}

void VulkanContext::BindBuffer(Descriptor* descriptor, const Buffer* buffer, uint32 dynamicOffset)
{
    BindBuffer(descriptor->set, descriptor->binding, buffer, dynamicOffset);
}

void VulkanContext::BindBuffer(uint32 set, uint32 binding, const Buffer* generalBuffer, uint32 dynamicOffset)
{
    auto buffer = Get(generalBuffer);
    LC_ASSERT(buffer->type == BufferType::kUniformDynamic);

    auto& bound = m_BoundSets[set];
    bound.bindings[binding] = { Binding::kUniformBufferDynamic, buffer };
    // TODO: Ensure order of dynamic offsets matches order in descriptor set
    bound.dynamicOffsets.push_back(dynamicOffset);
    bound.dirty = true;
}

void VulkanContext::BindTexture(Descriptor* descriptor, const Texture* texture, int level)
{
    auto& bound = m_BoundSets[descriptor->set];
    bound.bindings[descriptor->binding] = Binding{ Binding::kTexture, texture, level };
    bound.dirty = true;
}

void VulkanContext::BindImage(Descriptor* descriptor, const Texture* texture, int level)
{
    auto& bound = m_BoundSets[descriptor->set];
    bound.bindings[descriptor->binding] = Binding{ Binding::kImage, texture, level };
    bound.dirty = true;
}

void VulkanContext::Uniform(Descriptor* descriptor, const uint8* data, size_t size)
{
    LC_ASSERT(descriptor->size == size);

    auto offset = GetScratchOffset(descriptor->set, descriptor->binding);
    m_ScratchUniformBuffer->Upload(data, size, offset + descriptor->offset);
}

void VulkanContext::Uniform(Descriptor* descriptor, uint32 arrayIndex, const uint8* data, size_t size)
{
    LC_ASSERT(descriptor->size == size);

    auto offset = GetScratchOffset(descriptor->set, descriptor->binding);
    m_ScratchUniformBuffer->Upload(data, size, offset + descriptor->offset + arrayIndex * size);
}

void VulkanContext::EstablishBindings()
{
    LC_ASSERT(m_BoundPipeline != nullptr);

    for (int setIndex = 0; setIndex < m_BoundSets.size(); ++setIndex)
    {
        auto& set = m_BoundSets[setIndex];
        if (set.dirty)
        {
            // Find/alloc descriptor set
            auto& setLayouts = m_BoundPipeline->shader->setLayouts;
            auto layout = setLayouts[setIndex];
            auto setHandle = FindDescriptorSet(set.bindings, layout);

            // Bind set
            vkCmdBindDescriptorSets(m_CommandBuffer,
                GetBindPoint(m_BoundPipeline->type),
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

size_t VulkanContext::BindingHash::operator()(const VulkanContext::BindingArray& bindings) const
{
    return HashBytes<size_t>(bindings);
}

VkDescriptorSet VulkanContext::FindDescriptorSet(const BindingArray& bindings, VkDescriptorSetLayout layout)
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
        LC_CHECK(vkAllocateDescriptorSets(m_Device.m_Handle, &allocInfo, &set));

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

            switch (binding.type)
            {
            case Binding::kNone:
                continue;

            case Binding::kUniformBuffer:
            {
                bufferInfo = &bufferWrites.emplace_back(VkDescriptorBufferInfo{
                    .buffer = binding.buffer->handle,
                    .offset = 0,
                    .range = VK_WHOLE_SIZE
                });
                descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            }

            case Binding::kUniformBufferDynamic:
            {
                bufferInfo = &bufferWrites.emplace_back(VkDescriptorBufferInfo{
                    .buffer = binding.buffer->handle,
                    .offset = 0,
                    .range = VK_WHOLE_SIZE
                });
                descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                break;
            }

            case Binding::kStorageBuffer:
            {
                bufferInfo = &bufferWrites.emplace_back(VkDescriptorBufferInfo{
                    .buffer = binding.buffer->handle,
                    .offset = 0,
                    .range = VK_WHOLE_SIZE
                });
                descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            }

            case Binding::kTexture:
            {
                auto view = binding.level >= 0 ?
                    binding.texture->mipViews[binding.level] :
                    binding.texture->imageView;

                imageInfo = &imageWrites.emplace_back(VkDescriptorImageInfo{
                    .sampler = binding.texture->sampler,
                    .imageView = view,
                    .imageLayout = binding.texture->usage == TextureUsage::kReadWrite ?
                        VK_IMAGE_LAYOUT_GENERAL :
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                });
                descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            }

            case Binding::kImage:
            {
                auto view = binding.level >= 0 ?
                    binding.texture->mipViews[binding.level] :
                    binding.texture->imageView;

                imageInfo = &imageWrites.emplace_back(VkDescriptorImageInfo{
                    .sampler = binding.texture->sampler,
                    .imageView = view,
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL
                });
                descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                break;
            }
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

uint32 VulkanContext::GetScratchOffset(uint32 set, uint32 binding)
{
    if (!m_ScratchUniformBuffer)
    {
        m_ScratchUniformBuffer = m_Device.CreateBuffer(BufferType::kUniformDynamic, 65536);
    }

    auto matchBinding = [=](auto& arg)
    {
        return arg.set == set && arg.binding == binding;
    };

    auto it = std::find_if(m_ScratchBindings.begin(), m_ScratchBindings.end(), matchBinding);

    if (it == m_ScratchBindings.end())
    {
        // Create a new scratch binding for this block
        LC_ASSERT(m_BoundPipeline);
        auto& blocks = m_BoundPipeline->shader->blocks;
        auto block = std::find_if(blocks.begin(), blocks.end(), matchBinding);
        LC_ASSERT(block != blocks.end());

        auto offset = m_ScratchBindings.empty() ?
            m_ScratchDrawOffset :
            m_ScratchBindings.back().offset + m_ScratchBindings.back().size;

        // Align offset to required boundary
        auto alignment = m_Device.m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
        offset += (alignment - (offset % alignment)) % alignment;

        // Clear new allocation
        m_ScratchUniformBuffer->Clear(block->size, offset);

        // Bind to uniform block
        BindBuffer(set, binding, m_ScratchUniformBuffer, offset);

        it = &m_ScratchBindings.emplace_back(ScratchBinding{
            .set = set,
            .binding = binding,
            .offset = offset,
            .size = block->size
        });
    }
    return it->offset;
}

void VulkanContext::ResetScratchBindings()
{
    if (!m_ScratchBindings.empty())
    {
        m_ScratchDrawOffset = m_ScratchBindings.back().offset + m_ScratchBindings.back().size;
        m_ScratchBindings.clear();

        // TODO: REMOVE THIS
        if (m_ScratchDrawOffset > 60000)
        {
            m_ScratchUniformBuffer = m_Device.CreateBuffer(BufferType::kUniformDynamic, 65536);
            m_ScratchDrawOffset = 0;
        }
    }
}

const Pipeline* VulkanContext::BoundPipeline()
{
    LC_ASSERT(m_BoundPipeline);
    return m_BoundPipeline;
}

void VulkanContext::TransitionLayout(const Texture* generalTexture, VkPipelineStageFlags stage,
    VkAccessFlags access, VkImageLayout layout, uint32 layer, uint32 level)
{
    auto texture = Get(generalTexture);

    VkPipelineStageFlags srcStage{};
    VkAccessFlags srcAccess{};
    VkImageLayout srcLayout{};
    texture->SyncSrc(srcStage, srcAccess, srcLayout);

    auto barrier = VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srcAccess,
        .dstAccessMask = access,
        .oldLayout = srcLayout,
        .newLayout = layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture->image,
        .subresourceRange = {
            .aspectMask = texture->aspect,
            .baseMipLevel = (level == VK_REMAINING_MIP_LEVELS) ? 0 : level,
            .levelCount   = (level == VK_REMAINING_MIP_LEVELS) ? VK_REMAINING_MIP_LEVELS : 1,
            .baseArrayLayer = (layer == VK_REMAINING_ARRAY_LAYERS) ? 0 : layer,
            .layerCount     = (layer == VK_REMAINING_ARRAY_LAYERS) ? VK_REMAINING_ARRAY_LAYERS : 1
        }
    };

    vkCmdPipelineBarrier(m_CommandBuffer, srcStage, stage, VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanContext::RestoreLayout(const Texture* texture, VkPipelineStageFlags stage,
    VkAccessFlags access, VkImageLayout layout, uint32 layer, uint32 level)
{
    auto tex = Get(texture);

    VkPipelineStageFlags dstStage{};
    VkAccessFlags dstAccess{};
    VkImageLayout dstLayout{};
    tex->SyncDst(dstStage, dstAccess, dstLayout);

    auto barrier = VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = access,
        .dstAccessMask = dstAccess,
        .oldLayout = layout,
        .newLayout = dstLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = tex->image,
        .subresourceRange = {
            .aspectMask = tex->aspect,
            .baseMipLevel = (level == VK_REMAINING_MIP_LEVELS) ? 0 : level,
            .levelCount   = (level == VK_REMAINING_MIP_LEVELS) ? VK_REMAINING_MIP_LEVELS : 1,
            .baseArrayLayer = (layer == VK_REMAINING_ARRAY_LAYERS) ? 0 : layer,
            .layerCount     = (layer == VK_REMAINING_ARRAY_LAYERS) ? VK_REMAINING_ARRAY_LAYERS : 1
        }
    };

    vkCmdPipelineBarrier(m_CommandBuffer, stage, dstStage, VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr, 0, nullptr, 1, &barrier);
}

Device* VulkanContext::GetDevice()
{
    return &m_Device;
}

bool VulkanContext::Binding::operator==(const Binding& rhs) const
{
    return type == rhs.type && data == rhs.data && level == rhs.level;
}

}
