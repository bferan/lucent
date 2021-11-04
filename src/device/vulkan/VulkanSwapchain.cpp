#include "VulkanSwapchain.hpp"

#include "GLFW/glfw3.h"

#include "VulkanDevice.hpp"

namespace lucent
{

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device)
    : m_Device(device)
{
    LC_INFO("Creating new swapchain");

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->m_PhysicalDevice, device->m_Surface, &surfaceCapabilities);

    // Choose suitable sRGB surface format
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32 surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->m_PhysicalDevice, device->m_Surface, &surfaceFormatCount, nullptr);
    surfaceFormats.resize(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->m_PhysicalDevice,
        device->m_Surface,
        &surfaceFormatCount,
        surfaceFormats.data());

    VkSurfaceFormatKHR chosenFormat;
    for (auto& format: surfaceFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = format;
            break;
        }
    }

    // Choose suitable present mode
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Choose suitable extent
    VkExtent2D chosenExtent = surfaceCapabilities.currentExtent;
    if (chosenExtent.width == UINT32_MAX)
    {
        int w, h;
        glfwGetFramebufferSize(device->m_Window, &w, &h);

        chosenExtent = { static_cast<uint32>(w), static_cast<uint32>(h) };

        auto& min = surfaceCapabilities.minImageExtent;
        auto& max = surfaceCapabilities.maxImageExtent;

        chosenExtent.width = std::clamp(chosenExtent.width, min.width, max.width);
        chosenExtent.height = std::clamp(chosenExtent.height, min.height, max.height);
    }

    auto chosenImageCount = surfaceCapabilities.minImageCount + 1;

    auto sharedQueue = device->m_GraphicsQueue.familyIndex == device->m_PresentQueue.familyIndex;
    uint32 queueIndices[] = { device->m_GraphicsQueue.familyIndex, device->m_PresentQueue.familyIndex };

    // Create swapchain
    auto swapchainCreateInfo = VkSwapchainCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = device->m_Surface,
        .minImageCount = chosenImageCount,
        .imageFormat = chosenFormat.format,
        .imageColorSpace = chosenFormat.colorSpace,
        .imageExtent = chosenExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = sharedQueue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = sharedQueue ? 0u : 2u,
        .pQueueFamilyIndices = sharedQueue ? nullptr : queueIndices,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = chosenPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };
    LC_CHECK(vkCreateSwapchainKHR(device->GetHandle(), &swapchainCreateInfo, nullptr, &m_Handle));

    // Retrieve images from swapchain
    uint32 imageCount;
    vkGetSwapchainImagesKHR(device->GetHandle(), m_Handle, &imageCount, nullptr);
    std::vector<VkImage> images;
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device->GetHandle(), m_Handle, &imageCount, images.data());

    auto textureSettings = TextureSettings{
        .width = chosenExtent.width,
        .height = chosenExtent.height,
        .usage = TextureUsage::kPresentSrc
    };

    // Initialize swapchain textures
    m_Textures.resize(imageCount);
    m_AcquiredImage.resize(imageCount);
    m_ImageReady.resize(imageCount);
    for (int i = 0; i < imageCount; ++i)
    {
        m_Textures[i] = std::make_unique<VulkanTexture>(device, textureSettings, images[i], chosenFormat.format);

        // Create semaphores
        auto semaphoreInfo = VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };
        LC_CHECK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &m_AcquiredImage[i]));
        LC_CHECK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &m_ImageReady[i]));
    }
}

VulkanSwapchain::~VulkanSwapchain()
{
    LC_INFO("Destroying swapchain");

    for (int i = 0; i < m_Textures.size(); ++i)
    {
        m_Textures[i].reset();
        vkDestroySemaphore(m_Device->GetHandle(), m_AcquiredImage[i], nullptr);
        vkDestroySemaphore(m_Device->GetHandle(), m_ImageReady[i], nullptr);
    }
    vkDestroySwapchainKHR(m_Device->GetHandle(), m_Handle, nullptr);
}

Texture* VulkanSwapchain::AcquireImage(uint32 frame)
{
    auto index = FrameToImageSyncIndex(frame);

    LC_CHECK(vkAcquireNextImageKHR(m_Device->GetHandle(),
        m_Handle,
        UINT64_MAX,
        m_AcquiredImage[index],
        VK_NULL_HANDLE,
        &m_CurrentImageIndex));

    return m_Textures[m_CurrentImageIndex].get();
}

void VulkanSwapchain::SyncSubmit(uint32 frame, VkSubmitInfo& info)
{
    auto index = FrameToImageSyncIndex(frame);

    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &m_AcquiredImage[index];

    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &m_ImageReady[index];
}

bool VulkanSwapchain::Present(uint32 frame, VkQueue queue)
{
    auto index = FrameToImageSyncIndex(frame);

    auto presentInfo = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_ImageReady[index],
        .swapchainCount = 1,
        .pSwapchains = &m_Handle,
        .pImageIndices = &m_CurrentImageIndex
    };
    auto presentResult = vkQueuePresentKHR(queue, &presentInfo);

    return presentResult == VK_SUCCESS;
}

uint32 VulkanSwapchain::FrameToImageSyncIndex(uint32 frame) const
{
    return frame % m_Textures.size();
}

}

