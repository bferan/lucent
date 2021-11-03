#pragma once

#include "VulkanCommon.hpp"
#include "device/Texture.hpp"

namespace lucent
{

struct VulkanTexture : public Texture
{
public:
    VulkanTexture(VulkanDevice* device, const TextureSettings& settings, VkImage existingImage = VK_NULL_HANDLE);
    ~VulkanTexture();

    void Upload(size_t size, const void* data) override;

    VkImageLayout GetStartingLayout() const;

    void SyncSrc(VkPipelineStageFlags& stage, VkAccessFlags& access, VkImageLayout& layout) const;
    void SyncDst(VkPipelineStageFlags& stage, VkAccessFlags& access, VkImageLayout& layout) const;

public:
    VulkanDevice* device;
    VkImage image{};
    VkImageView imageView{};
    std::vector<VkImageView> mipViews;
    VkSampler sampler{};
    VmaAllocation alloc{};

    uint32 levels;
    VkSampleCountFlagBits samples;
    VkFormat format;
    VkExtent2D extent{};
    VkImageAspectFlags aspect;
};

}
