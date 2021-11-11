#pragma once

#include "VulkanCommon.hpp"
#include "device/Device.hpp"
#include "debug/Input.hpp"
#include "VulkanSwapchain.hpp"

struct GLFWwindow;

namespace lucent
{

class VulkanDevice : public Device
{
public:
    explicit VulkanDevice(GLFWwindow* window);
    ~VulkanDevice() override;

    Buffer* CreateBuffer(BufferType type, size_t size) override;
    void DestroyBuffer(Buffer* buffer) override;

    Texture* CreateTexture(const TextureSettings& textureSettings) override;
    void DestroyTexture(Texture* texture) override;

    Pipeline* CreatePipeline(const PipelineSettings& settings) override;
    void DestroyPipeline(Pipeline* pipeline) override;
    void ReloadPipelines() override;

    Framebuffer* CreateFramebuffer(const FramebufferSettings& settings) override;
    void DestroyFramebuffer(Framebuffer* framebuffer) override;

    Context* CreateContext() override;
    void DestroyContext(Context* context) override;
    void Submit(Context* context) override;

    Texture* AcquireSwapchainImage() override;
    bool Present() override;

    void WaitIdle() override;
    void RebuildSwapchain() override;

    VkDevice GetHandle() const { return m_Handle; }
    VmaAllocator GetAllocator() const { return m_Allocator; }
    const VkPhysicalDeviceLimits& GetLimits() const { return m_DeviceProperties.limits; }
    VkPhysicalDevice GetPhysicalHandle() const { return m_PhysicalDevice; }
    VkSurfaceKHR GetSurface() const { return m_Surface; }

private:
    friend class VulkanContext;
    friend class VulkanSwapchain;
    friend class VulkanTexture;

    void CreateInstance();
    void CreateDevice();

    template<typename T, typename C>
    void RemoveResource(T resource, C& container);

    static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT types,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

private:
    GLFWwindow* m_Window;
    VkInstance m_Instance{};
    VkDevice m_Handle{};
    VkDebugUtilsMessengerEXT m_DebugMessenger{};
    VkSurfaceKHR m_Surface{};
    VkPhysicalDevice m_PhysicalDevice{};
    VkPhysicalDeviceProperties m_DeviceProperties{};
    VmaAllocator m_Allocator{};

    struct DeviceQueue
    {
        VkQueue handle;
        uint32 familyIndex;
    };
    DeviceQueue m_GraphicsQueue{};
    DeviceQueue m_PresentQueue{};

    std::vector<std::unique_ptr<VulkanPipeline>> m_Pipelines;
    std::vector<std::unique_ptr<VulkanBuffer>> m_Buffers;
    std::vector<std::unique_ptr<VulkanTexture>> m_Textures;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_Framebuffers;
    std::vector<std::unique_ptr<VulkanContext>> m_Contexts;

    std::unique_ptr<VulkanSwapchain> m_Swapchain;
    bool m_SwapchainImageAcquired = false;
    uint64 m_FrameIndex{};

    Buffer* m_TransferBuffer;
    VulkanContext* m_OneShotContext;

    std::unique_ptr<ShaderCache> m_ShaderCache;
};

}
