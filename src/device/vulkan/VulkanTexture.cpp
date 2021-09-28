#include "VulkanTexture.hpp"

#include "device/vulkan/VulkanDevice.hpp"
#include "device/vulkan/VulkanContext.hpp"

namespace lucent
{

void VulkanTexture::Upload(size_t size, const void* data)
{
    auto& ctx = *device->m_OneShotContext;
    auto buffer = device->m_TransferBuffer;
    buffer->Upload(data, size);

    ctx.Begin();
    ctx.CopyTexture(buffer, 0, this, 0, 0, width, height);

    if (settings.generateMips)
        ctx.GenerateMips(this);

    ctx.End();
    device->Submit(&ctx);
}

VkImageLayout VulkanTexture::StartingLayout() const
{
    switch (usage)
    {
    case TextureUsage::kReadOnly:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case TextureUsage::kReadWrite:
        return VK_IMAGE_LAYOUT_GENERAL;
    case TextureUsage::kPresentSrc:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case TextureUsage::kDepthAttachment:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}

void VulkanTexture::SyncSrc(VkPipelineStageFlags& stage, VkAccessFlags& access, VkImageLayout& layout) const
{
    layout = StartingLayout();

    switch (usage)
    {
    case TextureUsage::kReadOnly:
    {
        stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        stage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        access |= VK_ACCESS_SHADER_READ_BIT;

        stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    }
    case TextureUsage::kReadWrite:
    {
        stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        stage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        access |= VK_ACCESS_SHADER_READ_BIT;
        access |= VK_ACCESS_SHADER_WRITE_BIT;

        stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    }
    case TextureUsage::kPresentSrc:
    {
        stage |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        access |= VK_ACCESS_NONE_KHR;
        break;
    }
    case TextureUsage::kDepthAttachment:
    {
        stage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        stage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    }
    }
}
void VulkanTexture::SyncDst(VkPipelineStageFlags& stage, VkAccessFlags& access, VkImageLayout& layout) const
{
    layout = StartingLayout();

    switch (usage)
    {
    case TextureUsage::kReadOnly:
    {
        stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        stage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        access |= VK_ACCESS_SHADER_READ_BIT;

        stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    }
    case TextureUsage::kReadWrite:
    {
        stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        stage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        access |= VK_ACCESS_SHADER_READ_BIT;
        access |= VK_ACCESS_SHADER_WRITE_BIT;

        stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    }
    case TextureUsage::kPresentSrc:
    {
        stage |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        access |= VK_ACCESS_NONE_KHR;
        layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        break;
    }
    case TextureUsage::kDepthAttachment:
    {
        stage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        stage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        break;
    }
    }
}

}
