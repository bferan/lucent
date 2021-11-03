#pragma once

#include "device/Framebuffer.hpp"
#include "VulkanCommon.hpp"

namespace lucent
{

class VulkanFramebuffer : public Framebuffer
{
public:
    VulkanFramebuffer(VulkanDevice* device, const FramebufferSettings& settings);
    ~VulkanFramebuffer();

public:
    VulkanDevice* device;
    VkFramebuffer handle;
    VkRenderPass renderPass;
    VkExtent2D extent;
    uint32 samples;

    Array<VkImageView, kMaxColorAttachments> colorImageViews;
    VkImageView depthImageView{};
};

}
