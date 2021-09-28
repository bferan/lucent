#pragma once

#include "VulkanCommon.hpp"
#include "device/Device.hpp"
#include "debug/Input.hpp"

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

private:
    void CreateInstance();
    void CreateDevice();

    void CreateSwapchain();
    void DestroySwapchain();

    uint32 GetSwapchainIndex() const;

    std::unique_ptr<VulkanPipeline> CreateGraphicsPipeline(const PipelineSettings& settings, Shader* shader);
    std::unique_ptr<VulkanPipeline> CreateComputePipeline(const PipelineSettings& settings, Shader* shader);
    void FreePipeline(VulkanPipeline* pipeline);

    static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT types,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

    friend class VulkanContext;

private:
    struct DeviceQueue
    {
        VkQueue handle;
        uint32 familyIndex;
    };

    struct Swapchain
    {
        VkSwapchainKHR handle;
        std::vector<VulkanTexture> textures;
        std::vector<VkSemaphore> acquiredImage;
        std::vector<VkSemaphore> imageReady;
    };

public:
    GLFWwindow* m_Window;
    VkInstance m_Instance{};
    VkDevice m_Handle{};
    VkDebugUtilsMessengerEXT m_DebugMessenger{};
    VkSurfaceKHR m_Surface{};
    VkPhysicalDevice m_PhysicalDevice{};
    VkPhysicalDeviceProperties m_DeviceProperties{};

    VmaAllocator m_Allocator{};

    DeviceQueue m_GraphicsQueue{};
    DeviceQueue m_PresentQueue{};

    Swapchain m_Swapchain;

    std::vector<std::unique_ptr<VulkanPipeline>> m_Pipelines;
    std::vector<std::unique_ptr<VulkanBuffer>> m_Buffers;
    std::vector<std::unique_ptr<VulkanTexture>> m_Textures;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_Framebuffers;
    std::vector<std::unique_ptr<VulkanContext>> m_Contexts;

    uint32 m_SwapchainImageIndex{};
    uint64 m_FrameIndex{};
    bool m_SwapchainImageAcquired = false;

    Buffer* m_TransferBuffer;
    VulkanContext* m_OneShotContext;

    std::unique_ptr<ShaderCache> m_ShaderCache;
};

}
