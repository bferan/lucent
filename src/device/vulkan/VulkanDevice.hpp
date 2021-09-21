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

    const Framebuffer* AcquireFramebuffer() override;

    Context* CreateContext() override;
    void Submit(Context* context, bool sync) override;
    void DestroyContext(Context* context) override;
    void Present() override;

private:
    void CreateInstance();
    void CreateDevice();
    void CreateSwapchain();

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
        std::vector<VulkanFramebuffer> framebuffers;
        std::vector<VulkanTexture> textures;
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

    VkSemaphore m_ImageAvailable{};
    VkSemaphore m_RenderFinished{};
    uint32 m_NextImageIndex{};

    Buffer* m_TransferBuffer;
    VkCommandPool m_OneShotCmdPool{};
    Context* m_OneShotContext;

    std::unique_ptr<ShaderCache> m_ShaderCache;
};

}
