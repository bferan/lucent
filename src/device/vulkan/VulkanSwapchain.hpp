#pragma once

#include "VulkanCommon.hpp"
#include "VulkanTexture.hpp"

namespace lucent
{

class VulkanSwapchain
{
public:
    explicit VulkanSwapchain(VulkanDevice* device);
    ~VulkanSwapchain();

    Texture* AcquireImage(uint32 frame);
    void SyncSubmit(uint32 frame, VkSubmitInfo& info);
    bool Present(uint32 frame, VkQueue queue);

    uint32 FrameToImageSyncIndex(uint32 frame) const;

private:
    VulkanDevice* m_Device;
    VkSwapchainKHR m_Handle{};

    std::vector<std::unique_ptr<VulkanTexture>> m_Textures;
    uint32 m_CurrentImageIndex = 0;

    std::vector<VkSemaphore> m_AcquiredImage;
    std::vector<VkSemaphore> m_ImageReady;
};

}
