
#pragma once

#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

#include "device/Input.hpp"
#include "device/Shader.hpp"

struct GLFWwindow;

namespace lucent
{

class Device;
class Shader;
class ShaderCache;
class Context;
struct Texture;
struct Framebuffer;

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector4 tangent;
    Vector2 texCoord0;
    Color color = Color::White();
};

struct DeviceQueue
{
    VkQueue handle;
    uint32 familyIndex;
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
    bool depthClampEnable = false;
};

struct Pipeline
{
    explicit Pipeline(PipelineSettings settings)
        : settings(std::move(settings)), shader(), handle()
    {}

    PipelineSettings settings;
    Shader* shader;
    VkPipeline handle;
};

enum class TextureFormat
{
    kR8,
    kRGBA8_SRGB,
    kRGBA8,
    kRGBA32F,
    kRG32F,
    kR32F,

    kDepth16U,
    kDepth32F
};

enum class TextureShape
{
    k2D,
    k2DArray,
    kCube
};

enum class TextureAddressMode
{
    kRepeat,
    kClampToEdge,
    kClampToBorder
};

struct Texture
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation alloc;

    VkExtent2D extent;
    VkFormat format;
    uint32 samples;
    VkImageAspectFlags aspect;

    VkSampler sampler;
    TextureFormat texFormat;
};

struct TextureSettings
{
    uint32 width = 1;
    uint32 height = 1;
    uint32 levels = 1;
    uint32 layers = 1;
    uint32 samples = 1;
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
    int colorLayer = -1;

    Texture* depthTexture = nullptr;
    int depthLayer = -1;
};

struct Framebuffer
{
    VkFramebuffer handle;
    VkRenderPass renderPass;
    VkExtent2D extent;
    uint32 samples;

    FramebufferUsage usage;

    Texture* colorTexture;
    VkImageView colorImageView;

    Texture* depthTexture;
    VkImageView depthImageView;
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
    UniformDynamic,
    Staging
};

struct Buffer
{
    void Upload(const void* data, size_t size, size_t offset = 0) const;
    void Clear(size_t size, size_t offset) const;

    Device* device;
    VkBuffer handle;
    VmaAllocation allocation;
    BufferType type;
    size_t bufSize;
};

class Device
{
public:
    Device();
    ~Device();

    Pipeline* CreatePipeline(const PipelineSettings& settings);
    void ReloadPipelines();

    Buffer* CreateBuffer(BufferType type, size_t size);

    Texture* CreateTexture(const TextureSettings& info, size_t size = 0, const void* data = nullptr);

    Framebuffer* CreateFramebuffer(const FramebufferSettings& info);

    const Framebuffer* AcquireFramebuffer();

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

    static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT types,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

public:
    GLFWwindow* m_Window;
    VkInstance m_Instance{};
    VkDebugUtilsMessengerEXT m_DebugMessenger{};
    VkSurfaceKHR m_Surface{};
    VkPhysicalDevice m_PhysicalDevice{};
    VkPhysicalDeviceProperties m_DeviceProperties{};

    VmaAllocator m_Allocator{};

    VkDevice m_Handle{};
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
    uint32 m_NextImageIndex{};

    Buffer* m_TransferBuffer;
    VkCommandPool m_OneShotCmdPool;

    std::unique_ptr<Input> m_Input;

    std::unique_ptr<ShaderCache> m_ShaderCache;

public:
    void CreateCube();
    void CreateQuad();

    struct Cube
    {
        Buffer* vertices;
        Buffer* indices;
        uint32 numIndices;
    } m_Cube;

    struct Quad
    {
        Buffer* vertices;
        Buffer* indices;
        uint32 numIndices;
    } m_Quad;

    Texture* m_BlackTexture;
    Texture* m_WhiteTexture;
    Texture* m_GrayTexture;
    Texture* m_GreenTexture;
    Texture* m_NormalTexture;

};

}
