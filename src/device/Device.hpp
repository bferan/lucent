
#pragma once

#include <vector>
#include <string>
#include <memory>

#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

#include "core/Vector3.hpp"
#include "core/Vector4.hpp"
#include "core/Color.hpp"
#include "device/Input.hpp"

struct GLFWwindow;

namespace lucent
{

class Device;
class Shader;
class ShaderCache;
struct Texture;
struct Framebuffer;

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
    Vector3 bitangent;
    TexCoord texCoord0;
    PackedColor color = Color::White().Pack();
};

struct DeviceQueue
{
    VkQueue handle;
    uint32_t familyIndex;
};

enum class ShaderStage
{
    kVertex,
    kFragment,
    kCompute
};

struct PipelineSettings
{
    std::string shaderName;

    Framebuffer* framebuffer = nullptr;
    bool depthTestEnable = true;
};

struct Pipeline
{
    static constexpr uint32_t kMaxSets = 4;

public:
    explicit Pipeline(PipelineSettings settings)
        : settings(std::move(settings))
    {
    }

public:
    PipelineSettings settings;

    Shader* shader{};
    VkPipeline handle{};
    VkDescriptorSetLayout setLayouts[kMaxSets]{};
    VkPipelineLayout layout{};
};

enum class TextureFormat
{
    kR8,
    kRGBA8_SRGB,
    kRGBA8,
    kRGBA32F,
    kDepth
};

enum class TextureShape
{
    k2D,
    kCube
};

enum class TextureAddressMode
{
    kRepeat,
    kClampToEdge
};

struct Texture
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation alloc;

    VkExtent2D extent;
    VkFormat format;

    VkSampler sampler;
    TextureFormat texFormat;
};

struct TextureSettings
{
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t levels = 1;
    TextureFormat format = TextureFormat::kRGBA8;
    TextureShape shape = TextureShape::k2D;
    TextureAddressMode addressMode = TextureAddressMode::kRepeat;
    bool generateMips = false;
};

enum class FramebufferUsage
{
    SwapchainImage,
    Default
};

struct FramebufferSettings
{
    FramebufferUsage usage = FramebufferUsage::Default;
    Texture* colorTexture = nullptr;
    Texture* depthTexture = nullptr;
};

struct Framebuffer
{
    VkFramebuffer handle;
    VkRenderPass renderPass;
    VkExtent2D extent;

    FramebufferUsage usage;

    Texture* colorTexture;
    Texture* depthTexture;
};

struct Swapchain
{
    VkSwapchainKHR handle;
    std::vector<Framebuffer> framebuffers;
    std::vector<Texture> textures;
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
    {
    }

    void Begin() const;
    void End() const;

    void BeginRenderPass(const Framebuffer& fbuffer, VkExtent2D extent = {});
    void EndRenderPass() const;

    void Bind(Pipeline& pipeline);

    // TODO: One entrypoint, determine binding point from buffer type
    void Bind(const Buffer* indexBuffer);
    void Bind(const Buffer* vertexBuffer, uint32_t binding);

    void BindSet(const DescriptorSet* set);
    void BindSet(const DescriptorSet* set, uint32_t dynamicOffset);

    void Draw(uint32_t indexCount) const;

    void CopyTexture(
        Texture* src, int srcLayer, int srcLevel,
        Texture* dst, int dstLayer, int dstLevel,
        uint32_t width, uint32_t height);

public:
    Device& m_Device;

    VkCommandPool m_CommandPool{};
    VkCommandBuffer m_CommandBuffer{};

    // Active state
    bool m_SwapchainWritten{};
    Pipeline* m_BoundPipeline{};
};

class Device
{
public:
    Device();
    ~Device();

    Pipeline* CreatePipeline(const PipelineSettings& settings);
    void ReloadPipelines();

    Buffer* CreateBuffer(BufferType type, size_t size);

    Texture* CreateTexture(const TextureSettings& info, size_t size = 0, void* data = nullptr);

    Framebuffer* CreateFramebuffer(const FramebufferSettings& info);

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

    std::unique_ptr<Pipeline> CreatePipeline(const PipelineSettings& settings, Shader* shader);
    void FreePipeline(Pipeline* pipeline);

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
    std::vector<std::unique_ptr<Framebuffer>> m_Framebuffers;
    std::vector<std::unique_ptr<Context>> m_Contexts;

    VkSemaphore m_ImageAvailable{};
    VkSemaphore m_RenderFinished{};
    uint32_t m_NextImageIndex{};

    Buffer* m_TransferBuffer;
    VkCommandPool m_OneShotCmdPool;

    VkDescriptorPool m_DescPool{};
    std::vector<std::unique_ptr<DescriptorSet>> m_DescSets;

    std::unique_ptr<Input> m_Input;

    std::unique_ptr<ShaderCache> m_ShaderCache;

public:
    void CreateCube();
    void CreateQuad();

    struct Cube
    {
        Buffer* vertices;
        Buffer* indices;
        uint32_t numIndices;
    } m_Cube;

    struct Quad
    {
        Buffer* vertices;
        Buffer* indices;
        uint32_t numIndices;
    } m_Quad;

    Texture* m_BlackTexture;
    Texture* m_WhiteTexture;
    Texture* m_GrayTexture;
    Texture* m_GreenTexture;

};

}
