
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

enum class PipelineType
{
    kGraphics,
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
    explicit Pipeline(PipelineSettings settings, PipelineType pipelineType)
        : settings(std::move(settings)), type(pipelineType), shader(), handle()
    {}

    PipelineType type;
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

enum class TextureFilter
{
    kLinear,
    kNearest
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
    uint32 levels;

    VkSampler sampler;
    TextureFormat texFormat;

    std::vector<VkImageView> mipViews;
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
    TextureFilter filter = TextureFilter::kLinear;
    bool generateMips = false;
    bool write = false;
};

enum class FramebufferUsage
{
    kSwapchainImage,
    kDefault
};

struct Framebuffer
{
    static constexpr int kMaxColorAttachments = 8;
    static constexpr int kMaxAttachments = kMaxColorAttachments + 1;

    VkFramebuffer handle{};
    VkRenderPass renderPass{};
    VkExtent2D extent{};
    uint32 samples{};

    FramebufferUsage usage;

    Array<Texture*, kMaxColorAttachments> colorTextures;
    Array<VkImageView, kMaxColorAttachments> colorImageViews;

    Texture* depthTexture{};
    VkImageView depthImageView{};
};

struct FramebufferSettings
{
    FramebufferUsage usage = FramebufferUsage::kDefault;

    Array<Texture*, Framebuffer::kMaxColorAttachments> colorTextures = {};
    int colorLayer = -1;
    int colorLevel = -1;

    Texture* depthTexture = nullptr;
    int depthLayer = -1;
    int depthLevel = -1;
};

struct Swapchain
{
    VkSwapchainKHR handle;
    std::vector<Framebuffer> framebuffers;
    std::vector<Texture> textures;
};

enum class BufferType
{
    kVertex,
    kIndex,
    kUniform,
    kUniformDynamic,
    kStorage,
    kStaging
};

struct Buffer
{
    void Upload(const void* data, size_t size, size_t offset = 0) const;

    void Clear(size_t size, size_t offset) const;

    void* Map() const;

    void Flush(size_t size, size_t offset) const;

    void Invalidate(size_t size, size_t offset) const;

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

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Pipeline* CreatePipeline(const PipelineSettings& settings, PipelineType type = PipelineType::kGraphics);
    void ReloadPipelines();

    Buffer* CreateBuffer(BufferType type, size_t size);

    Texture* CreateTexture(const TextureSettings& info, size_t size = 0, const void* data = nullptr);

    Framebuffer* CreateFramebuffer(const FramebufferSettings& info);

    const Framebuffer* AcquireFramebuffer();

    Context* CreateContext();
    void Submit(Context* context, bool sync = true);

    void Present();

public:
    friend class Buffer;

private:
    void CreateInstance();
    void CreateDevice();
    void CreateSwapchain();

    std::unique_ptr<Pipeline> CreateGraphicsPipeline(const PipelineSettings& settings, Shader* shader);
    std::unique_ptr<Pipeline> CreateComputePipeline(const PipelineSettings& settings, Shader* shader);
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
    void CreateSphere();

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

    struct Sphere
    {
        Buffer* vertices;
        Buffer* indices;
        uint32 numIndices;
    } m_Sphere;

    Texture* m_BlackTexture;
    Texture* m_WhiteTexture;
    Texture* m_GrayTexture;
    Texture* m_GreenTexture;
    Texture* m_NormalTexture;

};

}
