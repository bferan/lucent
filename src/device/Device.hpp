#pragma once

#include <vector>
#include <string>
#include <memory>

#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

struct GLFWwindow;

namespace lucent
{

class Device;

struct DeviceQueue
{
    VkQueue handle;
    uint32_t familyIndex;
};

struct Framebuffer
{
    VkFramebuffer handle;
    VkRenderPass renderPass;
    VkExtent2D extent;
    VkFormat format;
    VkImage image;
    VkImageView imageView;
};

struct Swapchain
{
    VkSwapchainKHR handle;
    std::vector<Framebuffer> framebuffers;
};

struct PipelineInfo
{
    std::string vertexShader;
    std::string fragmentShader;
};

struct Pipeline
{
    VkPipeline handle;
    VkDescriptorSetLayout setLayout;
    VkPipelineLayout layout;
};


struct Texture
{

};

struct TextureInfo
{

};

enum class BufferType
{
    Vertex,
    Index,
    Uniform
};

struct Buffer
{
    void Upload(void* data, size_t size);

    Device* device;
    VkBuffer handle;
    VmaAllocation allocation;
    BufferType type;
};

class Context
{
public:
    void BeginRenderPass(const Framebuffer& fbuffer) const;
    void EndRenderPass() const;

    void Bind(const Pipeline& pipeline) const;
    void Bind(const Buffer& vertexBuffer, const Buffer& indexBuffer) const;

    //void BindSlot(int set, int idx, const Buffer& buffer);
    //void BindSlot(int set, int idx, const Texture& texture);

    void Draw(uint32_t indexCount) const;

public:
    VkCommandPool m_CommandPool;
    VkCommandBuffer m_CommandBuffer;
};

class Device
{
public:
    Device();
    ~Device();

    Pipeline* CreatePipeline(const PipelineInfo& info);

    Buffer* CreateBuffer(BufferType type, size_t size);

    //Texture* CreateTexture(const TextureInfo& info);

    const Framebuffer& AcquireFramebuffer();

    Context* CreateContext();
    void Submit(Context* context);

    void Present();


public:
    friend class Buffer;

private:
    void CreateInstance();
    void CreateDevice();
    void CreateSwapchain();

public:
    GLFWwindow* m_Window;
    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkPhysicalDevice m_PhysicalDevice{};

    VmaAllocator m_Allocator{};

    VkDevice m_Device{};
    DeviceQueue m_GraphicsQueue{};
    DeviceQueue m_PresentQueue{};

    Swapchain m_Swapchain;

    std::vector<std::unique_ptr<Pipeline>> m_Pipelines;
    std::vector<std::unique_ptr<Buffer>> m_Buffers;
    std::vector<std::unique_ptr<Context>> m_Contexts;

    VkSemaphore m_ImageAvailable;
    VkSemaphore m_RenderFinished;
    uint32_t m_NextImageIndex;

};

}
