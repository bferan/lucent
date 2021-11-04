#include "VulkanTexture.hpp"

#include "device/vulkan/VulkanDevice.hpp"
#include "device/vulkan/VulkanContext.hpp"

namespace lucent
{

static VkFormat TextureFormatToVkFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::kR8:
        return VK_FORMAT_R8_UNORM;

    case TextureFormat::kRG8:
        return VK_FORMAT_R8G8_UNORM;

    case TextureFormat::kRGB8:
        return VK_FORMAT_R8G8B8_UNORM;

    case TextureFormat::kRGBA8:
        return VK_FORMAT_R8G8B8A8_UNORM;

    case TextureFormat::kRGBA8_sRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case TextureFormat::kRGB10A2:
        return VK_FORMAT_A2R10G10B10_UNORM_PACK32;

    case TextureFormat::kR32F:
        return VK_FORMAT_R32_SFLOAT;

    case TextureFormat::kRG32F:
        return VK_FORMAT_R32G32_SFLOAT;

    case TextureFormat::kRGB32F:
        return VK_FORMAT_R32G32B32_SFLOAT;

    case TextureFormat::kRGBA32F:
        return VK_FORMAT_R32G32B32A32_SFLOAT;

    case TextureFormat::kDepth16U:
        return VK_FORMAT_D16_UNORM;

    case TextureFormat::kDepth32F:
        return VK_FORMAT_D32_SFLOAT;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}

static VkImageAspectFlags TextureFormatToAspect(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::kDepth16U:
    case TextureFormat::kDepth32F:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static VkImageUsageFlags TextureUsageToVkUsage(TextureUsage usage)
{
    VkImageUsageFlags flags = 0;

    switch (usage)
    {
    case TextureUsage::kReadOnly:
    {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        break;
    }
    case TextureUsage::kReadWrite:
    {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        break;
    }
    case TextureUsage::kDepthAttachment:
    {
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        break;
    }
    default:
        break;
    }
    return flags;
}

static VkImageViewType TextureShapeToViewType(TextureShape shape)
{
    switch (shape)
    {
    case TextureShape::k2D:
        return VK_IMAGE_VIEW_TYPE_2D;

    case TextureShape::k2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

    case TextureShape::kCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;

    default:
        LC_ASSERT(0 && "Invalid texture shape");
        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

static VkSamplerAddressMode TextureAddressModeToVk(TextureAddressMode mode)
{
    switch (mode)
    {
    case TextureAddressMode::kRepeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;

    case TextureAddressMode::kClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    case TextureAddressMode::kClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

    default:
        LC_ASSERT(0 && "Invalid address mode");
        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }
}

static VkFilter TextureFilterToVk(TextureFilter filter)
{
    switch (filter)
    {
    case TextureFilter::kLinear:
        return VK_FILTER_LINEAR;
    case TextureFilter::kNearest:
        return VK_FILTER_NEAREST;
    }
}

VulkanTexture::VulkanTexture(VulkanDevice* dev,
    const TextureSettings& info,
    VkImage existingImage,
    VkFormat existingFormat)
    : device(dev)
{
    m_Settings = info;
    auto deviceHandle = device->GetHandle();

    format = (existingFormat != VK_FORMAT_UNDEFINED) ? existingFormat : TextureFormatToVkFormat(info.format);
    aspect = TextureFormatToAspect(info.format);
    extent = { .width = info.width, .height = info.height };

    LC_ASSERT((info.samples & (info.samples - 1)) == 0 && "Requested non-power-of-2 samples");
    LC_ASSERT(info.samples > 0 && info.samples <= 64);
    samples = static_cast<VkSampleCountFlagBits>(info.samples);

    VkFlags flags = 0;
    uint32 arrayLayers = info.layers;
    if (info.shape == TextureShape::kCube)
    {
        arrayLayers = 6;
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        LC_ASSERT(info.width == info.height);
    }

    // Override specified levels if generating mip chain
    levels = info.levels;
    if (info.generateMips)
    {
        levels = (uint32)Floor(Log2((float)Max(info.width, info.height))) + 1;
    }

    // Create image
    image = existingImage;
    if (image == VK_NULL_HANDLE)
    {
        auto imageInfo = VkImageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = flags,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {
                .width = info.width,
                .height = info.height,
                .depth = 1
            },
            .mipLevels = levels,
            .arrayLayers = arrayLayers,
            .samples = samples,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = TextureUsageToVkUsage(info.usage),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        auto allocInfo = VmaAllocationCreateInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY
        };
        LC_CHECK(vmaCreateImage(device->GetAllocator(), &imageInfo, &allocInfo, &image, &alloc, nullptr));
    }

    // Create image view
    auto viewInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = TextureShapeToViewType(info.shape),
        .format = format,
        .components = {},
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = info.levels,
            .baseArrayLayer = 0,
            .layerCount = arrayLayers
        }
    };
    LC_CHECK(vkCreateImageView(deviceHandle, &viewInfo, nullptr, &imageView));

    // Create mip level views
    if (levels > 1)
    {
        mipViews.reserve(levels);
        for (int level = 0; level < levels; ++level)
        {
            auto mipInfo = viewInfo;
            mipInfo.subresourceRange.baseMipLevel = level;
            mipInfo.subresourceRange.levelCount = 1;

            VkImageView view;
            LC_CHECK(vkCreateImageView(deviceHandle, &mipInfo, nullptr, &view));

            mipViews.push_back(view);
        }
    }

    // Create sampler
    auto filter = TextureFilterToVk(info.filter);
    auto addressMode = TextureAddressModeToVk(info.addressMode);
    auto samplerInfo = VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = addressMode,
        .addressModeV = addressMode,
        .addressModeW = addressMode,
        .mipLodBias = 0,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = device->GetLimits().maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };
    LC_CHECK(vkCreateSampler(deviceHandle, &samplerInfo, nullptr, &sampler));

    // Transition image to starting layout
    if (auto startLayout = GetStartingLayout(); startLayout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        device->m_OneShotContext->Begin();

        auto imgBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE_KHR,
            .dstAccessMask = VK_ACCESS_NONE_KHR,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = startLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = aspect,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
        };
        vkCmdPipelineBarrier(device->m_OneShotContext->m_CommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imgBarrier);

        device->m_OneShotContext->End();
        device->Submit(device->m_OneShotContext);
    }
}

VulkanTexture::~VulkanTexture()
{
    vkDestroyImageView(device->GetHandle(), imageView, nullptr);
    vkDestroySampler(device->GetHandle(), sampler, nullptr);

    if (m_Settings.usage != TextureUsage::kPresentSrc)
    {
        vmaDestroyImage(device->GetAllocator(), image, alloc);
    }
}

void VulkanTexture::Upload(size_t size, const void* data)
{
    auto& settings = GetSettings();

    auto& ctx = *device->m_OneShotContext;
    auto buffer = device->m_TransferBuffer;
    buffer->Upload(data, size, 0);

    ctx.Begin();
    ctx.CopyTexture(buffer, 0, this, 0, 0, settings.width, settings.height);

    if (settings.generateMips)
        ctx.GenerateMips(this);

    ctx.End();
    device->Submit(&ctx);
}

VkImageLayout VulkanTexture::GetStartingLayout() const
{
    switch (GetSettings().usage)
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
    layout = GetStartingLayout();

    switch (GetSettings().usage)
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
    layout = GetStartingLayout();

    switch (GetSettings().usage)
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
