#include "Device.hpp"

#include "GLFW/glfw3.h"
#include "glslang/Public/ShaderLang.h"

#include "device/ShaderCache.hpp"
#include "device/Context.hpp"

namespace lucent
{

// Device
static VkBufferUsageFlags BufferTypeToFlags(BufferType type)
{
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    switch (type)
    {

    case BufferType::kVertex:
    {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    }
    case BufferType::kIndex:
    {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    }
    case BufferType::kUniform:
    case BufferType::kUniformDynamic:
    {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;
    }
    case BufferType::kStaging:
    {
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        break;
    }
    case BufferType::kStorage:
    {
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;
    }

    }
    return flags;
}

static VkDescriptorType BufferTypeToDescriptorType(BufferType type)
{
    switch (type)
    {
    case BufferType::kUniform:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    case BufferType::kUniformDynamic:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    case BufferType::kStorage:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    default:
        LC_ASSERT(false);
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

static VkFormat TextureFormatToVkFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::kR8:
        return VK_FORMAT_R8_UNORM;

    case TextureFormat::kRGBA8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case TextureFormat::kRGBA8:
        return VK_FORMAT_R8G8B8A8_UNORM;

    case TextureFormat::kR32F:
        return VK_FORMAT_R32_SFLOAT;

    case TextureFormat::kRG32F:
        return VK_FORMAT_R32G32_SFLOAT;

    case TextureFormat::kRGBA32F:
        return VK_FORMAT_R32G32B32A32_SFLOAT;

    case TextureFormat::kDepth16U:
        return VK_FORMAT_D16_UNORM;

    case TextureFormat::kDepth32F:
        return VK_FORMAT_D32_SFLOAT;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}

static VkImageAspectFlags TextureFormatToAspect(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::kDepth16U:
    case TextureFormat::kDepth32F:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static VkImageUsageFlags TextureFormatToUsage(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::kR8:
    case TextureFormat::kRGBA8_SRGB:
    case TextureFormat::kRGBA8:
    case TextureFormat::kRG32F:
    case TextureFormat::kR32F:
    case TextureFormat::kRGBA32F:
        // TODO: Narrow this down
        return VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    case TextureFormat::kDepth32F:
    case TextureFormat::kDepth16U:
        return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    default:
        LC_ASSERT(0 && "Invalid texture format");
        return 0;
    }
}

static VkImageViewType TextureShapeToViewType(TextureShape shape)
{
    switch (shape)
    {
    case TextureShape::k2D:
        return VK_IMAGE_VIEW_TYPE_2D;

    case TextureShape::k2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

    case TextureShape::kCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;

    default:
        LC_ASSERT(0 && "Invalid texture shape");
        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

static VkSamplerAddressMode TextureAddressModeToVk(TextureAddressMode mode)
{
    switch (mode)
    {
    case TextureAddressMode::kRepeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;

    case TextureAddressMode::kClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    case TextureAddressMode::kClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

    default:
        LC_ASSERT(0 && "Invalid address mode");
        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }
}

static VkFilter TextureFilterToVk(TextureFilter filter)
{
    switch (filter)
    {
    case TextureFilter::kLinear:
        return VK_FILTER_LINEAR;
    case TextureFilter::kNearest:
        return VK_FILTER_NEAREST;
    }
}

void Device::CreateCube()
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    vertices.assign({
        { .position = { -1.0f, -1.0f, -1.0f }},
        { .position = { -1.0f, -1.0f, 1.0f }},
        { .position = { 1.0f, -1.0f, 1.0f }},
        { .position = { 1.0f, -1.0f, -1.0f }},
        { .position = { -1.0f, 1.0f, -1.0f }},
        { .position = { -1.0f, 1.0f, 1.0f }},
        { .position = { 1.0f, 1.0f, 1.0f }},
        { .position = { 1.0f, 1.0f, -1.0f }},
    });

    indices.assign({
        0, 1, 2, 2, 3, 0,
        0, 4, 5, 5, 1, 0,
        0, 3, 7, 7, 4, 0,
        2, 1, 5, 5, 6, 2,
        3, 2, 6, 6, 7, 3,
        4, 7, 6, 6, 5, 4
    });

    m_Cube.vertices = CreateBuffer(BufferType::kVertex, vertices.size() * sizeof(Vertex));
    m_Cube.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex));

    m_Cube.indices = CreateBuffer(BufferType::kIndex, indices.size() * sizeof(uint32));
    m_Cube.indices->Upload(indices.data(), indices.size() * sizeof(uint32));
    m_Cube.numIndices = indices.size();
}

void Device::CreateQuad()
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    vertices.assign({
        { .position = { -1.0f, -1.0f, 0.0f }, .texCoord0 = { 0.0f, 0.0f }},
        { .position = { -1.0f, 1.0f, 0.0f }, .texCoord0 = { 0.0f, 1.0f }},
        { .position = { 1.0f, 1.0f, 0.0f }, .texCoord0 = { 1.0f, 1.0f }},
        { .position = { 1.0f, -1.0f, 0.0f }, .texCoord0 = { 1.0f, 0.0f }}
    });

    indices.assign({ 0, 1, 2, 2, 3, 0 });

    m_Quad.vertices = CreateBuffer(BufferType::kVertex, vertices.size() * sizeof(Vertex));
    m_Quad.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex));

    m_Quad.indices = CreateBuffer(BufferType::kIndex, indices.size() * sizeof(uint32));
    m_Quad.indices->Upload(indices.data(), indices.size() * sizeof(uint32));
    m_Quad.numIndices = indices.size();
}

void Device::CreateSphere()
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    constexpr int kNumSegments = 32;
    constexpr int kNumRings = 16;

    for (int ring = 0; ring <= kNumRings; ++ring)
    {
        auto theta = (float)ring * kPi / kNumRings;
        for (int segment = 0; segment < kNumSegments; ++segment)
        {
            auto phi = (float)segment * k2Pi / kNumSegments;
            auto pos = Vector3(Sin(theta) * Cos(phi), Cos(theta), Sin(theta) * Sin(phi));

            vertices.push_back(Vertex{
                .position = pos,
                .normal = pos,
                .texCoord0 = { (float)segment / kNumSegments, (float)ring / kNumRings }
            });
        }
    }

    for (int ring = 1; ring <= kNumRings; ++ring)
    {
        for (int segment = 0; segment < kNumSegments; ++segment)
        {
            int start = ring * kNumSegments;
            int prev = start - kNumSegments;

            int pos = start + segment;

            indices.push_back(pos);
            indices.push_back(pos - kNumSegments);
            indices.push_back(((pos - kNumSegments + 1) % kNumSegments) + prev);

            indices.push_back(((pos - kNumSegments + 1) % kNumSegments) + prev);
            indices.push_back(((pos + 1) % kNumSegments) + start);
            indices.push_back(pos);
        }
    }

    m_Sphere.vertices = CreateBuffer(BufferType::kVertex, vertices.size() * sizeof(Vertex));
    m_Sphere.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex));

    m_Sphere.indices = CreateBuffer(BufferType::kIndex, indices.size() * sizeof(uint32));
    m_Sphere.indices->Upload(indices.data(), indices.size() * sizeof(uint32));
    m_Sphere.numIndices = indices.size();
}

Device::Device()
{
    // Set up GLFW
    if (!glfwInit())
        return;

    glslang::InitializeProcess();

    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(
        1600, 900,
        "Lucent",
        nullptr, nullptr);

    LC_ASSERT(m_Window != nullptr);

    // Init input
    m_Input = std::make_unique<Input>(m_Window);

    CreateInstance();
    glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface);
    CreateDevice();

    // Initialize VMA
    auto allocatorInfo = VmaAllocatorCreateInfo{
        .physicalDevice = m_PhysicalDevice,
        .device = m_Handle,
        .instance = m_Instance,
        .vulkanApiVersion = VK_API_VERSION_1_2
    };
    vmaCreateAllocator(&allocatorInfo, &m_Allocator);

    // Create semaphores and fences
    auto semaphoreInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    vkCreateSemaphore(m_Handle, &semaphoreInfo, nullptr, &m_ImageAvailable);
    vkCreateSemaphore(m_Handle, &semaphoreInfo, nullptr, &m_RenderFinished);

    // Create transfer buffer
    m_TransferBuffer = CreateBuffer(BufferType::kStaging, 128 * 1024 * 1024);

    auto poolInfo = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = m_GraphicsQueue.familyIndex
    };
    auto result = vkCreateCommandPool(m_Handle, &poolInfo, nullptr, &m_OneShotCmdPool);
    LC_ASSERT(result == VK_SUCCESS);

    // Create shader cache
    m_ShaderCache = std::make_unique<ShaderCache>(this);

    CreateSwapchain();

    // Create dummy textures
    uint32 black = 0xff000000;
    m_BlackTexture = CreateTexture(TextureSettings{}, sizeof(black), &black);

    uint32 white = 0xffffffff;
    m_WhiteTexture = CreateTexture(TextureSettings{}, sizeof(white), &white);

    uint32 gray = 0xff808080;
    m_GrayTexture = CreateTexture(TextureSettings{}, sizeof(gray), &gray);

    uint32 normal = Color(0.5f, 0.5f, 1.0f).Pack();
    m_NormalTexture = CreateTexture(TextureSettings{}, sizeof(normal), &normal);

    uint32 green = 0xff00ff00;
    m_GreenTexture = CreateTexture(TextureSettings{}, sizeof(green), &green);

    // Create primitive meshes
    CreateCube();
    CreateQuad();
    CreateSphere();
}

Device::~Device()
{
    vkDeviceWaitIdle(m_Handle);

    // Destroy pipelines
    for (auto& pipeline: m_Pipelines)
    {
        FreePipeline(pipeline.get());
    }

    // Destroy framebuffers
    for (auto& fb: m_Framebuffers)
    {
        for (auto view: fb->colorImageViews)
        {
            if (view) vkDestroyImageView(m_Handle, view, nullptr);
        }
        if (fb->depthImageView) vkDestroyImageView(m_Handle, fb->depthImageView, nullptr);

        vkDestroyFramebuffer(m_Handle, fb->handle, nullptr);
        vkDestroyRenderPass(m_Handle, fb->renderPass, nullptr);
    }

    // Destroy buffers
    for (auto& buff: m_Buffers)
    {
        vmaDestroyBuffer(m_Allocator, buff->handle, buff->allocation);
    }

    // Destroy textures
    for (auto& tex: m_Textures)
    {
        vkDestroyImageView(m_Handle, tex->imageView, nullptr);
        vmaDestroyImage(m_Allocator, tex->image, tex->alloc);
        vkDestroySampler(m_Handle, tex->sampler, nullptr);
    }

    // Destroy contexts
    for (auto& ctx: m_Contexts)
    {
        ctx->Dispose();
    }

    // Destroy shaders
    m_ShaderCache->Clear();

    // Destroy swapchain
    vkDestroyRenderPass(m_Handle, m_Swapchain.framebuffers[0].renderPass, nullptr);
    for (auto& tex: m_Swapchain.textures)
    {
        vkDestroyImageView(m_Handle, tex.imageView, nullptr);
    }
    for (auto& fb: m_Swapchain.framebuffers)
    {
        vkDestroyFramebuffer(m_Handle, fb.handle, nullptr);
    }
    vkDestroySwapchainKHR(m_Handle, m_Swapchain.handle, nullptr);

    // Destroy semaphores
    vkDestroySemaphore(m_Handle, m_ImageAvailable, nullptr);
    vkDestroySemaphore(m_Handle, m_RenderFinished, nullptr);

    // Destroy one shot command pool
    vkDestroyCommandPool(m_Handle, m_OneShotCmdPool, nullptr);

    // VMA
    vmaDestroyAllocator(m_Allocator);

    vkDestroyDevice(m_Handle, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

    // Destroy debug messenger
    auto destroyMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyMessenger)
        destroyMessenger(m_Instance, m_DebugMessenger, nullptr);

    vkDestroyInstance(m_Instance, nullptr);

    glslang::FinalizeProcess();

    glfwTerminate();
}

void Device::CreateInstance()
{
    std::vector<const char*> instanceExtensions = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    uint32 glfwExtCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    instanceExtensions.insert(instanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtCount);

    std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    auto appInfo = VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Lucent Demo",
        .pEngineName = "Lucent Engine"
    };

    auto createInfo = VkInstanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()
    };

    auto result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
    LC_ASSERT(result == VK_SUCCESS);

    // Create debug messenger
    auto debugMessengerInfo = VkDebugUtilsMessengerCreateInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

        .pfnUserCallback = DebugCallback,
        .pUserData = this
    };

    auto createMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (createMessenger)
    {
        result = createMessenger(m_Instance, &debugMessengerInfo, nullptr, &m_DebugMessenger);
        LC_ASSERT(result == VK_SUCCESS);
    }
}

void Device::CreateDevice()
{
    // Find suitable physical device
    uint32 deviceCount;
    std::vector<VkPhysicalDevice> physicalDevices;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    physicalDevices.resize(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, physicalDevices.data());

    VkPhysicalDevice selectedDevice;
    for (auto& device: physicalDevices)
    {
        vkGetPhysicalDeviceProperties(device, &m_DeviceProperties);

        if (m_DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            selectedDevice = device;
            break;
        }
    }

    LC_ASSERT(selectedDevice != nullptr);
    m_PhysicalDevice = selectedDevice;

    // Find queue families for graphics and present
    uint32 queueFamilyCount;
    std::vector<VkQueueFamilyProperties> familyProperties;
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, nullptr);
    familyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, familyProperties.data());

    uint32 graphicsFamilyIdx = -1;
    uint32 presentFamilyIdx = -1;
    for (int i = 0; i < familyProperties.size(); ++i)
    {
        const auto& family = familyProperties[i];

        VkBool32 graphics = family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
        VkBool32 present;
        vkGetPhysicalDeviceSurfaceSupportKHR(selectedDevice, i, m_Surface, &present);

        if (graphics && present)
        {
            graphicsFamilyIdx = presentFamilyIdx = i;
            break;
        }
        else if (graphics)
        {
            graphicsFamilyIdx = i;
        }
        else if (present)
        {
            presentFamilyIdx = i;
        }
    }

    float queuePriority = 1.0f;
    std::set<uint32> familyIndices = { graphicsFamilyIdx, presentFamilyIdx };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(familyIndices.size());
    for (auto idx: familyIndices)
    {
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = idx,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    auto deviceFeatures = VkPhysicalDeviceFeatures{
        .depthClamp = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };

    // Create device
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    auto deviceCreateInfo = VkDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    auto result = vkCreateDevice(selectedDevice, &deviceCreateInfo, nullptr, &m_Handle);
    LC_ASSERT(result == VK_SUCCESS);

    m_GraphicsQueue.familyIndex = graphicsFamilyIdx;
    m_PresentQueue.familyIndex = presentFamilyIdx;

    vkGetDeviceQueue(m_Handle, graphicsFamilyIdx, 0, &m_GraphicsQueue.handle);
    vkGetDeviceQueue(m_Handle, presentFamilyIdx, 0, &m_PresentQueue.handle);
}

void Device::CreateSwapchain()
{
    // Find suitable format, extent and present mode
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities);

    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32 surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, nullptr);
    surfaceFormats.resize(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, surfaceFormats.data());

    VkSurfaceFormatKHR chosenFormat;
    for (auto& format: surfaceFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = format;
            break;
        }
    }

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkExtent2D chosenExtent = surfaceCapabilities.currentExtent;
    if (chosenExtent.width == UINT32_MAX)
    {
        int w, h;
        glfwGetFramebufferSize(m_Window, &w, &h);

        chosenExtent = { static_cast<uint32>(w), static_cast<uint32>(h) };

        auto& min = surfaceCapabilities.minImageExtent;
        auto& max = surfaceCapabilities.maxImageExtent;

        chosenExtent.width = std::clamp(chosenExtent.width, min.width, max.width);
        chosenExtent.height = std::clamp(chosenExtent.height, min.height, max.height);
    }

    auto chosenImageCount = surfaceCapabilities.minImageCount + 1;

    auto sharedQueue = m_GraphicsQueue.familyIndex == m_PresentQueue.familyIndex;
    uint32 queueIndices[] = { m_GraphicsQueue.familyIndex, m_PresentQueue.familyIndex };

    // Create swapchain
    auto swapchainCreateInfo = VkSwapchainCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_Surface,
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

    auto result = vkCreateSwapchainKHR(m_Handle, &swapchainCreateInfo, nullptr, &m_Swapchain.handle);
    LC_ASSERT(result == VK_SUCCESS);

    // Retrieve images from swapchain
    uint32 imageCount;
    vkGetSwapchainImagesKHR(m_Handle, m_Swapchain.handle, &imageCount, nullptr);
    std::vector<VkImage> images;
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Handle, m_Swapchain.handle, &imageCount, images.data());

    // Create render pass
    auto colAttachmentDescription = VkAttachmentDescription{
        .format = chosenFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    auto depthAttachmentDescription = VkAttachmentDescription{
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    auto colAttachmentReference = VkAttachmentReference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    auto depthAttachmentReference = VkAttachmentReference{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    auto subpassDesc = VkSubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colAttachmentReference,
        .pDepthStencilAttachment = &depthAttachmentReference
    };

    VkAttachmentDescription attachments[] = { colAttachmentDescription, depthAttachmentDescription };

    auto renderPassInfo = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpassDesc,
    };

    VkRenderPass renderPass;
    result = vkCreateRenderPass(m_Handle, &renderPassInfo, nullptr, &renderPass);
    LC_ASSERT(result == VK_SUCCESS);

    // Create depth texture
    auto depthTexture = CreateTexture(TextureSettings{
        .width = chosenExtent.width,
        .height = chosenExtent.height,
        .format = TextureFormat::kDepth32F });

    // Initialize swapchain textures & framebuffers
    m_Swapchain.textures.resize(imageCount);
    m_Swapchain.framebuffers.resize(imageCount);
    for (int i = 0; i < imageCount; ++i)
    {
        auto& tex = m_Swapchain.textures[i];
        auto& fb = m_Swapchain.framebuffers[i];

        tex.image = images[i];
        tex.alloc = VK_NULL_HANDLE;
        tex.extent = chosenExtent;
        tex.format = chosenFormat.format;
        tex.aspect = VK_IMAGE_ASPECT_COLOR_BIT;

        // Create image view
        auto viewCreateInfo = VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = tex.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = tex.format,
            .components = VkComponentMapping{},
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        result = vkCreateImageView(m_Handle, &viewCreateInfo, nullptr, &tex.imageView);
        LC_ASSERT(result == VK_SUCCESS);

        fb.samples = 1;
        fb.extent = chosenExtent;
        fb.renderPass = renderPass;
        fb.usage = FramebufferUsage::kSwapchainImage;
        fb.colorTextures = { &tex };
        fb.depthTexture = depthTexture;

        // Create framebuffer
        VkImageView fbAttachments[] = { fb.colorTextures.front()->imageView, fb.depthTexture->imageView };

        auto framebufferInfo = VkFramebufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = fb.renderPass,
            .attachmentCount = LC_ARRAY_SIZE(fbAttachments),
            .pAttachments = fbAttachments,
            .width = tex.extent.width,
            .height = tex.extent.height,
            .layers = 1
        };
        result = vkCreateFramebuffer(m_Handle, &framebufferInfo, nullptr, &fb.handle);
        LC_ASSERT(result == VK_SUCCESS);
    }
}

Pipeline* Device::CreatePipeline(const PipelineSettings& settings, PipelineType type)
{
    auto shader = m_ShaderCache->Compile(settings.shaderName);

    auto pipeline = type == PipelineType::kGraphics ?
        CreateGraphicsPipeline(settings, shader) :
        CreateComputePipeline(settings, shader);

    return m_Pipelines.emplace_back(std::move(pipeline)).get();
}

std::unique_ptr<Pipeline> Device::CreateGraphicsPipeline(const PipelineSettings& settings, Shader* shaderPtr)
{
    auto pipelinePtr = std::make_unique<Pipeline>(settings, PipelineType::kGraphics);

    if (!shaderPtr) return pipelinePtr;

    auto& shader = *shaderPtr;
    auto& pipeline = *pipelinePtr;
    auto& framebuffer = settings.framebuffer ? *settings.framebuffer : m_Swapchain.framebuffers[0];

    pipeline.shader = shaderPtr;
    shader.uses++;

    // Populate pipeline shader stages
    VkPipelineShaderStageCreateInfo stageInfos[Shader::kMaxStages];
    for (int i = 0; i < shader.stages.size(); ++i)
    {
        auto& stage = shader.stages[i];
        stageInfos[i] = VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage.stageBit,
            .module = stage.module,
            .pName = "main"
        };
    }

    // Configure pipeline fixed functions
    VkVertexInputBindingDescription vertexBindingInfos[] = {
        {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    VkVertexInputAttributeDescription vertexAttributeInfos[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, normal)
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, tangent)
        },
        {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, texCoord0)
        },
        {
            .location = 4,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, color)
        }
    };

    auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = LC_ARRAY_SIZE(vertexBindingInfos),
        .pVertexBindingDescriptions = vertexBindingInfos,
        .vertexAttributeDescriptionCount = LC_ARRAY_SIZE(vertexAttributeInfos),
        .pVertexAttributeDescriptions = vertexAttributeInfos
    };

    auto inputAssemblyInfo = VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    auto viewport = VkViewport{
        .x = 0.0f, .y = 0.0f,
        .width = static_cast<float>(framebuffer.extent.width),
        .height = static_cast<float>(framebuffer.extent.height),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };

    auto scissor = VkRect2D{
        .offset = {},
        .extent = framebuffer.extent
    };

    auto viewportStateInfo = VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    auto rasterizationInfo = VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = static_cast<VkBool32>(settings.depthClampEnable),
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    auto multisampleInfo = VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = static_cast<VkSampleCountFlagBits>(framebuffer.samples),
        .sampleShadingEnable = VK_FALSE
    };

    auto depthStencilInfo = VkPipelineDepthStencilStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = static_cast<VkBool32>(settings.depthTestEnable ? VK_TRUE : VK_FALSE),
        .depthWriteEnable = static_cast<VkBool32>(settings.depthTestEnable ? VK_TRUE : VK_FALSE),
        .depthCompareOp = settings.depthTestEnable ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    Array <VkPipelineColorBlendAttachmentState, Framebuffer::kMaxColorAttachments> colorAttachments;
    for (int i = 0; i < framebuffer.colorTextures.size(); ++i)
    {
        auto colorBlendAttachmentInfo = VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT
        };
        if (!settings.depthTestEnable)
        {
            colorBlendAttachmentInfo = VkPipelineColorBlendAttachmentState{
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                    | VK_COLOR_COMPONENT_A_BIT
            };
        }
        colorAttachments.push_back(colorBlendAttachmentInfo);
    }

    auto colorBlendInfo = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pAttachments = colorAttachments.data()
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT
    };

    auto dynamicInfo = VkPipelineDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = LC_ARRAY_SIZE(dynamicStates),
        .pDynamicStates = dynamicStates
    };

    auto pipelineCreateInfo = VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32>(shader.stages.size()),
        .pStages = stageInfos,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizationInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = &depthStencilInfo,
        .pColorBlendState = &colorBlendInfo,
        .pDynamicState = &dynamicInfo,
        .layout = shader.pipelineLayout,
        .renderPass = framebuffer.renderPass,
        .subpass = 0
    };

    auto result = vkCreateGraphicsPipelines(m_Handle, VK_NULL_HANDLE,
        1, &pipelineCreateInfo, nullptr, &pipeline.handle);
    LC_ASSERT(result == VK_SUCCESS);

    return pipelinePtr;
}

std::unique_ptr<Pipeline> Device::CreateComputePipeline(const PipelineSettings& settings, Shader* shaderPtr)
{
    auto pipelinePtr = std::make_unique<Pipeline>(settings, PipelineType::kCompute);
    if (!shaderPtr) return pipelinePtr;

    auto& shader = *shaderPtr;
    auto& pipeline = *pipelinePtr;

    pipeline.shader = shaderPtr;
    shader.uses++;

    LC_ASSERT(shader.stages.size() == 1);
    auto& stage = shader.stages.back();

    auto computeInfo = VkComputePipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage.stageBit,
            .module = stage.module,
            .pName = "main"
        },
        .layout = shader.pipelineLayout
    };

    auto result = vkCreateComputePipelines(m_Handle, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &pipeline.handle);
    LC_ASSERT(result == VK_SUCCESS);

    return pipelinePtr;
}

void Device::ReloadPipelines()
{
    vkDeviceWaitIdle(m_Handle);
    for (auto& pipeline: m_Pipelines)
    {
        auto updatedShader = m_ShaderCache->Compile(pipeline->settings.shaderName);
        if (updatedShader)
        {
            if (pipeline->shader != updatedShader)
            {
                auto updatedPipeline = (pipeline->type == PipelineType::kGraphics) ?
                    CreateGraphicsPipeline(pipeline->settings, updatedShader) :
                    CreateComputePipeline(pipeline->settings, updatedShader);

                std::swap(*pipeline, *updatedPipeline);
                FreePipeline(updatedPipeline.get());
                m_ShaderCache->Release(updatedPipeline->shader);
            }
        }
    }
}

void Device::FreePipeline(Pipeline* pipeline)
{
    vkDestroyPipeline(m_Handle, pipeline->handle, nullptr);
}

Buffer* Device::CreateBuffer(BufferType type, size_t size)
{
    auto& buff = *m_Buffers.emplace_back(std::make_unique<Buffer>());

    buff.type = type;
    buff.device = this;
    buff.bufSize = size;

    auto bufferInfo = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = BufferTypeToFlags(type),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };

    auto result = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buff.handle, &buff.allocation, nullptr);
    LC_ASSERT(result == VK_SUCCESS);

    return &buff;
}

Texture* Device::CreateTexture(const TextureSettings& info, size_t size, const void* data)
{
    auto& tex = *m_Textures.emplace_back(std::make_unique<Texture>());

    auto format = TextureFormatToVkFormat(info.format);
    auto aspect = TextureFormatToAspect(info.format);
    auto usage = TextureFormatToUsage(info.format);
    auto viewType = TextureShapeToViewType(info.shape);
    auto addressMode = TextureAddressModeToVk(info.addressMode);
    auto filter = TextureFilterToVk(info.filter);

    LC_ASSERT(info.samples > 0 && info.samples <= 64);
    LC_ASSERT((info.samples & (info.samples - 1)) == 0 && "Requested non-power-of-2 samples");
    auto samples = (VkSampleCountFlagBits)info.samples;

    VkFlags flags = 0;
    uint32 arrayLayers = info.layers;

    if (info.shape == TextureShape::kCube)
    {
        arrayLayers = 6;
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        LC_ASSERT(info.width == info.height);
    }

    if (info.write)
    {
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    tex.samples = info.samples;
    tex.texFormat = info.format;
    tex.format = format;
    tex.extent = { .width = info.width, .height = info.height };
    tex.aspect = aspect;
    tex.levels = info.levels;

    // Create image
    auto imageInfo = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = info.width,
            .height = info.height,
            .depth = 1
        },
        .mipLevels = info.levels,
        .arrayLayers = arrayLayers,
        .samples = samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    auto result = vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &tex.image, &tex.alloc, nullptr);
    LC_ASSERT(result == VK_SUCCESS);

    // Create image view
    auto viewInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = tex.image,
        .viewType = viewType,
        .format = format,
        .components = {},
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = info.levels,
            .baseArrayLayer = 0,
            .layerCount = arrayLayers
        }
    };
    result = vkCreateImageView(m_Handle, &viewInfo, nullptr, &tex.imageView);
    LC_ASSERT(result == VK_SUCCESS);

    // Create mip level views
    if (tex.levels > 1)
    {
        tex.mipViews.reserve(tex.levels);
        for (int level = 0; level < tex.levels; ++level)
        {
            auto mipInfo = viewInfo;
            mipInfo.subresourceRange.baseMipLevel = level;
            mipInfo.subresourceRange.levelCount = 1;

            VkImageView view;
            result = vkCreateImageView(m_Handle, &mipInfo, nullptr, &view);
            LC_ASSERT(result == VK_SUCCESS);

            tex.mipViews.push_back(view);
        }
    }

    // Create sampler
    auto samplerInfo = VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = addressMode,
        .addressModeV = addressMode,
        .addressModeW = addressMode,
        .mipLodBias = 0,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = m_DeviceProperties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS, // TODO: Change for shadow maps
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    vkCreateSampler(m_Handle, &samplerInfo, nullptr, &tex.sampler);

    if (size > 0 && data)
    {
        // Stage texture
        m_TransferBuffer->Upload(data, size);

        // Alloc command buffer
        VkCommandBuffer cb;
        auto cbAllocInfo = VkCommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_OneShotCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        vkAllocateCommandBuffers(m_Handle, &cbAllocInfo, &cb);

        auto beginInfo = VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        vkBeginCommandBuffer(cb, &beginInfo);

        // Transition NONE -> DST_OPTIMAL
        auto imgDstBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE_KHR,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = tex.image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imgDstBarrier);

        // Transfer image
        auto imgCopyRegion = VkBufferImageCopy{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = {},
            .imageExtent = {
                .width = info.width,
                .height = info.height,
                .depth = 1
            }
        };
        vkCmdCopyBufferToImage(cb,
            m_TransferBuffer->handle,
            tex.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imgCopyRegion);

        if (info.generateMips)
        {
            // TODO: Generate mip levels here
        }

        // Transition DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        auto imgSrcBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = tex.image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imgSrcBarrier);

        vkEndCommandBuffer(cb);

        auto submitInfo = VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cb
        };
        result = vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
        LC_ASSERT(result == VK_SUCCESS);

        // Free command buffer
        vkQueueWaitIdle(m_GraphicsQueue.handle);
        vkFreeCommandBuffers(m_Handle, m_OneShotCmdPool, 1, &cb);
        vkResetCommandPool(m_Handle, m_OneShotCmdPool, 0);
    }

    return &tex;
}

Framebuffer* Device::CreateFramebuffer(const FramebufferSettings& info)
{
    auto& fb = *m_Framebuffers.emplace_back(std::make_unique<Framebuffer>());

    fb.usage = info.usage;
    fb.colorTextures = info.colorTextures;
    fb.depthTexture = info.depthTexture;

    fb.extent = info.colorTextures.empty() ? info.depthTexture->extent : info.colorTextures.front()->extent;
    fb.samples = info.colorTextures.empty() ? info.depthTexture->samples : info.colorTextures.front()->samples;

    // Create image views if a specific layer or level is requested
    auto createTempView = [&](Texture* texture, int layer, int level, VkImageAspectFlags aspectFlags) -> VkImageView
    {
        if (!texture || (layer < 0 && level < 0)) return VK_NULL_HANDLE;

        auto viewInfo = VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture->image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = texture->format,
            .subresourceRange = {
                .aspectMask = aspectFlags,
                .baseMipLevel = static_cast<uint32_t>(level < 0 ? 0 : level),
                .levelCount = 1,
                .baseArrayLayer = static_cast<uint32_t>(layer < 0 ? 0 : layer),
                .layerCount = 1
            }
        };

        VkImageView view;
        auto result = vkCreateImageView(m_Handle, &viewInfo, nullptr, &view);
        LC_ASSERT(result == VK_SUCCESS);

        return view;
    };

    for (auto texture: fb.colorTextures)
    {
        fb.colorImageViews
            .push_back(createTempView(texture, info.colorLayer, info.colorLevel, VK_IMAGE_ASPECT_COLOR_BIT));
    }
    fb.depthImageView = createTempView(info.depthTexture, info.depthLayer, info.depthLevel, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Create render pass

    // Internal convention used here is that all color attachments are placed at indices starting at 0, then the depth
    // attachment (if present) is placed at the end

    Array <VkAttachmentDescription, Framebuffer::kMaxAttachments> attachments;
    Array <VkImageView, Framebuffer::kMaxAttachments> imageViews;
    auto depthIndex = VK_ATTACHMENT_UNUSED;

    // Populate attachment descriptions
    for (int i = 0; i < fb.colorTextures.size(); ++i)
    {
        auto colorTexture = fb.colorTextures[i];
        auto colorView = fb.colorImageViews[i] ? fb.colorImageViews[i] : colorTexture->imageView;

        attachments.emplace_back(VkAttachmentDescription{
            .format = colorTexture->format,
            .samples = static_cast<VkSampleCountFlagBits>(colorTexture->samples),
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        });
        imageViews.push_back(colorView);
    }
    if (info.depthTexture)
    {
        depthIndex = attachments.size();
        attachments.emplace_back(VkAttachmentDescription{
            .format = info.depthTexture->format,
            .samples = static_cast<VkSampleCountFlagBits>(info.depthTexture->samples),
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        });
        imageViews.push_back(fb.depthImageView ? fb.depthImageView : info.depthTexture->imageView);
    }

    // Populate attachment references
    Array <VkAttachmentReference, Framebuffer::kMaxColorAttachments> colorRefs;
    for (uint32 i = 0; i < fb.colorTextures.size(); ++i)
    {
        colorRefs.push_back(VkAttachmentReference{
            .attachment = i,
            .layout = attachments[i].finalLayout
        });
    }
    auto depthRef = VkAttachmentReference{
        .attachment = depthIndex,
        .layout = (depthIndex != VK_ATTACHMENT_UNUSED) ? attachments[depthIndex].finalLayout : VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto subpassDesc = VkSubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorRefs.size()),
        .pColorAttachments = colorRefs.data(),
        .pDepthStencilAttachment = &depthRef
    };

    auto passInfo = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDesc
    };

    auto result = vkCreateRenderPass(m_Handle, &passInfo, nullptr, &fb.renderPass);
    LC_ASSERT(result == VK_SUCCESS);

    // Create framebuffer
    auto fbInfo = VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = fb.renderPass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = fb.extent.width,
        .height = fb.extent.height,
        .layers = 1
    };

    result = vkCreateFramebuffer(m_Handle, &fbInfo, nullptr, &fb.handle);
    LC_ASSERT(result == VK_SUCCESS);

    // Transition images to correct layout
    VkCommandBuffer cb;
    auto cbAllocInfo = VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_OneShotCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(m_Handle, &cbAllocInfo, &cb);

    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cb, &beginInfo);

    // Color
    for (auto colorTexture: fb.colorTextures)
    {
        auto colorBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE_KHR,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = colorTexture->image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
        };
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &colorBarrier);
    }

    // Depth
    if (info.depthTexture)
    {
        auto depthBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE_KHR,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = info.depthTexture->image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
        };
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &depthBarrier);
    }

    vkEndCommandBuffer(cb);

    auto submitInfo = VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cb
    };
    result = vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
    LC_ASSERT(result == VK_SUCCESS);

    vkQueueWaitIdle(m_GraphicsQueue.handle);
    vkFreeCommandBuffers(m_Handle, m_OneShotCmdPool, 1, &cb);
    vkResetCommandPool(m_Handle, m_OneShotCmdPool, 0);

    return &fb;
}

Context* Device::CreateContext()
{
    auto& ctx = *m_Contexts.emplace_back(std::make_unique<Context>(*this));
    return &ctx;
}

const Framebuffer* Device::AcquireFramebuffer()
{
    // Acquire image
    vkAcquireNextImageKHR(m_Handle,
        m_Swapchain.handle,
        UINT64_MAX,
        m_ImageAvailable,
        VK_NULL_HANDLE,
        &m_NextImageIndex);

    return &m_Swapchain.framebuffers[m_NextImageIndex];
}

void Device::Submit(Context* context, bool sync)
{
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    auto submitInfo = context->m_SwapchainWritten ?
        VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = sync ? 1u : 0u,
            .pWaitSemaphores = &m_ImageAvailable,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &context->m_CommandBuffer,
            .signalSemaphoreCount = sync ? 1u : 0u,
            .pSignalSemaphores = &m_RenderFinished
        } :
        VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &context->m_CommandBuffer
        };

    auto result = vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
    LC_ASSERT(result == VK_SUCCESS);

    // TODO: REMOVE
    vkQueueWaitIdle(m_GraphicsQueue.handle);
}

void Device::Present()
{
    // Present
    auto presentInfo = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_RenderFinished,
        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain.handle,
        .pImageIndices = &m_NextImageIndex
    };
    vkQueuePresentKHR(m_PresentQueue.handle, &presentInfo);
    vkDeviceWaitIdle(m_Handle);
}

VkBool32 Device::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT types,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        LC_INFO(callbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        LC_WARN(callbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        LC_ERROR(callbackData->pMessage);
        break;

    default:
        break;
    }
    return VK_FALSE;
}

// Buffer
void Buffer::Upload(const void* data, size_t size, size_t offset) const
{
    LC_ASSERT(offset + size <= bufSize);

    // TODO: Use staging buffer
    uint8* ptr;
    vmaMapMemory(device->m_Allocator, allocation, (void**)&ptr);
    ptr += offset;
    memcpy(ptr, data, size);
    vmaUnmapMemory(device->m_Allocator, allocation);
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

void Buffer::Clear(size_t size, size_t offset) const
{
    LC_ASSERT(offset + size <= bufSize);

    uint8* ptr;
    vmaMapMemory(device->m_Allocator, allocation, (void**)&ptr);
    ptr += offset;
    memset(ptr, 0, size);
    vmaUnmapMemory(device->m_Allocator, allocation);
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

void* Buffer::Map() const
{
    void* data;
    auto result = vmaMapMemory(device->m_Allocator, allocation, &data);
    LC_ASSERT(result == VK_SUCCESS);
    return data;
}

void Buffer::Flush(size_t size, size_t offset) const
{
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

void Buffer::Invalidate(size_t size, size_t offset) const
{
    vmaInvalidateAllocation(device->m_Allocator, allocation, offset, size);
}

}