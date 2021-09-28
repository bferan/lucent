#pragma once

#include "VulkanCommon.hpp"
#include "device/Texture.hpp"

namespace lucent
{

struct VulkanTexture : public Texture
{
public:
    void Upload(size_t size, const void* data) override;

    VkImageLayout StartingLayout() const;

    void SyncSrc(VkPipelineStageFlags& stage, VkAccessFlags& access, VkImageLayout& layout) const;
    void SyncDst(VkPipelineStageFlags& stage, VkAccessFlags& access, VkImageLayout& layout) const;

public:
    VulkanDevice* device;
    VkImage image;
    VkImageView imageView;
    VmaAllocation alloc;

    VkExtent2D extent;
    VkFormat format;
    uint32 samples;
    VkImageAspectFlags aspect;
    uint32 levels;

    VkSampler sampler;
    TextureFormat texFormat;
    bool initialized;

    std::vector<VkImageView> mipViews;
};

}
