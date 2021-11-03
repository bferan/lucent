#include "VulkanFramebuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTexture.hpp"

namespace lucent
{

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice* dev, const FramebufferSettings& info)
    : device(dev)
{
    m_Settings = info;

    extent = Get(info.colorTextures.empty() ? info.depthTexture : info.colorTextures.front())->extent;
    samples = Get(info.colorTextures.empty() ? info.depthTexture : info.colorTextures.front())->samples;

    // Create image views if a specific layer or level is requested
    auto createTempView =
        [&](VulkanTexture* texture, int layer, int level, VkImageAspectFlags aspectFlags) -> VkImageView
        {
            if (!texture || (layer < 0 && level < 0)) return VK_NULL_HANDLE;

            auto viewInfo = VkImageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = texture->image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = texture->format,
                .subresourceRange = {
                    .aspectMask = aspectFlags,
                    .baseMipLevel = static_cast<uint32_t>(level < 0 ? 0 : level),
                    .levelCount = 1,
                    .baseArrayLayer = static_cast<uint32_t>(layer < 0 ? 0 : layer),
                    .layerCount = 1
                }
            };

            VkImageView view;
            LC_CHECK(vkCreateImageView(device->GetHandle(), &viewInfo, nullptr, &view));

            return view;
        };

    for (auto texture: info.colorTextures)
    {
        colorImageViews.push_back(createTempView(
            Get(texture), info.colorLayer, info.colorLevel, VK_IMAGE_ASPECT_COLOR_BIT));
    }
    depthImageView = createTempView(
        Get(info.depthTexture), info.depthLayer, info.depthLevel, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Create render pass

    // Internal convention used here is that all color attachments are placed at indices starting at 0, then the depth
    // attachment (if present) is placed at the end

    Array <VkAttachmentDescription, kMaxAttachments> attachments;
    Array <VkImageView, kMaxAttachments> imageViews;
    auto depthIndex = VK_ATTACHMENT_UNUSED;

    // Populate attachment descriptions
    for (int i = 0; i < info.colorTextures.size(); ++i)
    {
        auto colorTexture = Get(info.colorTextures[i]);
        auto colorView = colorImageViews[i] ? colorImageViews[i] : colorTexture->imageView;

        attachments.emplace_back(VkAttachmentDescription{
            .format = colorTexture->format,
            .samples = static_cast<VkSampleCountFlagBits>(colorTexture->samples),
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        });
        imageViews.push_back(colorView);
    }
    if (info.depthTexture)
    {
        depthIndex = attachments.size();
        attachments.emplace_back(VkAttachmentDescription{
            .format = Get(info.depthTexture)->format,
            .samples = static_cast<VkSampleCountFlagBits>(Get(info.depthTexture)->samples),
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        });
        imageViews.push_back(depthImageView ? depthImageView : Get(info.depthTexture)->imageView);
    }

    // Populate attachment references
    Array <VkAttachmentReference, kMaxColorAttachments> colorRefs;
    for (uint32 i = 0; i < info.colorTextures.size(); ++i)
    {
        colorRefs.push_back(VkAttachmentReference{
            .attachment = i,
            .layout = attachments[i].finalLayout
        });
    }
    auto depthRef = VkAttachmentReference{
        .attachment = depthIndex,
        .layout = (depthIndex != VK_ATTACHMENT_UNUSED) ? attachments[depthIndex].finalLayout : VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto subpassDesc = VkSubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorRefs.size()),
        .pColorAttachments = colorRefs.data(),
        .pDepthStencilAttachment = &depthRef
    };

    auto passInfo = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDesc
    };
    LC_CHECK(vkCreateRenderPass(device->GetHandle(), &passInfo, nullptr, &renderPass));

    // Create framebuffer
    auto fbInfo = VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = extent.width,
        .height = extent.height,
        .layers = 1
    };
    LC_CHECK(vkCreateFramebuffer(device->GetHandle(), &fbInfo, nullptr, &handle));
}

VulkanFramebuffer::~VulkanFramebuffer()
{
    auto deviceHandle = device->GetHandle();
    for (auto view: colorImageViews)
    {
        if (view) vkDestroyImageView(deviceHandle, view, nullptr);
    }
    if (depthImageView) vkDestroyImageView(deviceHandle, depthImageView, nullptr);
    vkDestroyFramebuffer(deviceHandle, handle, nullptr);
    vkDestroyRenderPass(deviceHandle, renderPass, nullptr);
}

}
