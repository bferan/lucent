#pragma once

#include <vector>
#include <string>
#include <memory>

#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

#include "core/Vector3.hpp"
#include "core/Vector4.hpp"
#include "device/Input.hpp"

struct GLFWwindow;

namespace lucent
{

class Device;
class ShaderCache;
struct Texture;

struct TexCoord
{
    float u;
    float v;
};

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    TexCoord texCoord0;
};

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

    Texture* depthTexture;
};

struct Swapchain
{
    VkSwapchainKHR handle;
    std::vector<Framebuffer> framebuffers;
};

struct ProgramInfo
{
    std::string vertShader;
    std::string fragShader;
};

struct PipelineInfo
{
    ProgramInfo programInfo;
};

struct Pipeline
{
    static constexpr uint32_t kMaxSets = 4;

    VkPipeline handle;
    VkDescriptorSetLayout setLayouts[kMaxSets];
    VkPipelineLayout layout;
};

enum class TextureFormat
{
    RGBA,
    Depth
};

struct Texture
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation alloc;

    VkSampler sampler;
    TextureFormat format;
};

struct TextureInfo
{
    uint32_t width = 1;
    uint32_t height = 1;
    TextureFormat format = TextureFormat::RGBA;
};

enum class BufferType
{
    Vertex,
    Index,
    Uniform,
    Staging
};

struct Buffer
{
    void Upload(void* data, size_t size, size_t offset = 0) const;

    Device* device;
    VkBuffer handle;
    VmaAllocation allocation;
    BufferType type;
    size_t bufSize;
};

struct DescriptorSet
{
    VkDescriptorSet handle;
    uint32_t index;
};

class Context
{
public:
    explicit Context(Device& device)
        : m_Device(device)
    {}

    void Begin() const;
    void End() const;

    void BeginRenderPass(const Framebuffer& fbuffer) const;
    void EndRenderPass() const;

    void Bind(Pipeline& pipeline);

    void Bind(const Buffer* indexBuffer);
    void Bind(const Buffer* vertexBuffer, uint32_t binding);

    void BindSet(const DescriptorSet* set);
    void BindSet(const DescriptorSet* set, uint32_t dynamicOffset);

    void Draw(uint32_t indexCount) const;

public:
    Device& m_Device;

    VkCommandPool m_CommandPool{};
    VkCommandBuffer m_CommandBuffer{};

    // Active state
    Pipeline* m_BoundPipeline{};
};

class Device
{
public:
    Device();
    ~Device();

    Pipeline* CreatePipeline(const PipelineInfo& info);

    Buffer* CreateBuffer(BufferType type, size_t size);

    Texture* CreateTexture(const TextureInfo& info, size_t size = 0, void* data = nullptr);

    const Framebuffer& AcquireFramebuffer();

    DescriptorSet* CreateDescriptorSet(const Pipeline& pipeline, uint32_t set);
    void WriteSet(DescriptorSet* set, uint32_t binding, const Buffer& buffer);
    void WriteSet(DescriptorSet* set, uint32_t binding, const Texture& texture);

    Context* CreateContext();
    void Submit(Context* context);

    void Present();


public:
    friend class Buffer;

private:
    void CreateInstance();
    void CreateDevice();
    void CreateSwapchain();
    void CreateShader(const std::string& vertex, const std::string& frag);

public:
    GLFWwindow* m_Window;
    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkPhysicalDevice m_PhysicalDevice{};
    VkPhysicalDeviceProperties m_DeviceProperties{};

    VmaAllocator m_Allocator{};

    VkDevice m_Device{};
    DeviceQueue m_GraphicsQueue{};
    DeviceQueue m_PresentQueue{};

    Swapchain m_Swapchain;

    std::vector<std::unique_ptr<Pipeline>> m_Pipelines;
    std::vector<std::unique_ptr<Buffer>> m_Buffers;
    std::vector<std::unique_ptr<Texture>> m_Textures;
    std::vector<std::unique_ptr<Context>> m_Contexts;

    VkSemaphore m_ImageAvailable{};
    VkSemaphore m_RenderFinished{};
    uint32_t m_NextImageIndex{};

    Buffer* m_TransferBuffer;
    VkCommandPool m_OneShotCmdPool;

    VkDescriptorPool m_DescPool{};
    std::vector<std::unique_ptr<DescriptorSet>> m_DescSets;

    std::unique_ptr<Input> m_Input;

    Texture* m_DefaultTexture;

    std::unique_ptr<ShaderCache> m_ShaderCache;
};

}
