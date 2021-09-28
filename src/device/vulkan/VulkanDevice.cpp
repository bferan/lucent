#include "VulkanDevice.hpp"

#include "GLFW/glfw3.h"
#include "glslang/Public/ShaderLang.h"

#include "device/vulkan/ShaderCache.hpp"
#include "device/vulkan/VulkanBuffer.hpp"
#include "device/vulkan/VulkanTexture.hpp"
#include "device/vulkan/VulkanPipeline.hpp"
#include "device/vulkan/VulkanContext.hpp"
#include "device/vulkan/VulkanFramebuffer.hpp"
#include "device/vulkan/ShaderCache.hpp"
#include "rendering/Geometry.hpp"

#include "windows.h"
#include "renderdoc_app.h"

namespace lucent
{

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

    case TextureFormat::kRG8:
        return VK_FORMAT_R8G8_UNORM;

    case TextureFormat::kRGB8:
        return VK_FORMAT_R8G8B8_UNORM;

    case TextureFormat::kRGBA8:
        return VK_FORMAT_R8G8B8A8_UNORM;

    case TextureFormat::kRGBA8_sRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case TextureFormat::kRGB10A2:
        return VK_FORMAT_A2R10G10B10_UNORM_PACK32;

    case TextureFormat::kR32F:
        return VK_FORMAT_R32_SFLOAT;

    case TextureFormat::kRG32F:
        return VK_FORMAT_R32G32_SFLOAT;

    case TextureFormat::kRGB32F:
        return VK_FORMAT_R32G32B32_SFLOAT;

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

static VkImageUsageFlags TextureUsageToVkUsage(TextureUsage usage)
{
    VkImageUsageFlags flags = 0;

    switch (usage)
    {
    case TextureUsage::kReadOnly:
    {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        break;
    }
    case TextureUsage::kReadWrite:
    {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        break;
    }
    case TextureUsage::kDepthAttachment:
    {
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        break;
    }
    default:
        break;
    }
    return flags;
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

RENDERDOC_API_1_4_1* g_API;

VulkanDevice::VulkanDevice(GLFWwindow* window)
{
//    if(HMODULE mod = LoadLibraryA("renderdoc.dll"))
//    {
//        auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
//        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void **)&g_API);
//        LC_ASSERT(ret == 1);
//    }
//    g_API->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
//    g_API->SetCaptureFilePathTemplate("../lucent-captures/lucent-capture");

    m_Window = window;

    glslang::InitializeProcess();

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

    // Create transfer buffer
    m_TransferBuffer = VulkanDevice::CreateBuffer(BufferType::kStaging, 128 * 1024 * 1024);
    m_OneShotContext = Get(VulkanDevice::CreateContext());

    // Create shader cache
    m_ShaderCache = std::make_unique<ShaderCache>(this);

    CreateSwapchain();

    InitGeometry(this);
    m_Input = std::make_unique<Input>(m_Window);
}

VulkanDevice::~VulkanDevice()
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
    m_Contexts.clear();

    // Destroy shaders
    m_ShaderCache->Clear();

    DestroySwapchain();

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

void VulkanDevice::CreateInstance()
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
    LC_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance));

    // Create debug messenger
    auto debugMessengerInfo = VkDebugUtilsMessengerCreateInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
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
        LC_CHECK(createMessenger(m_Instance, &debugMessengerInfo, nullptr, &m_DebugMessenger));
    }
}

void VulkanDevice::CreateDevice()
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
    LC_CHECK(vkCreateDevice(selectedDevice, &deviceCreateInfo, nullptr, &m_Handle));

    m_GraphicsQueue.familyIndex = graphicsFamilyIdx;
    m_PresentQueue.familyIndex = presentFamilyIdx;

    vkGetDeviceQueue(m_Handle, graphicsFamilyIdx, 0, &m_GraphicsQueue.handle);
    vkGetDeviceQueue(m_Handle, presentFamilyIdx, 0, &m_PresentQueue.handle);
}

void VulkanDevice::CreateSwapchain()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities);

    // Choose suitable sRGB surface format
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

    // Choose suitable present mode
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Choose suitable extent
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
    LC_CHECK(vkCreateSwapchainKHR(m_Handle, &swapchainCreateInfo, nullptr, &m_Swapchain.handle));

    // Retrieve images from swapchain
    uint32 imageCount;
    vkGetSwapchainImagesKHR(m_Handle, m_Swapchain.handle, &imageCount, nullptr);
    std::vector<VkImage> images;
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Handle, m_Swapchain.handle, &imageCount, images.data());

    // Initialize swapchain textures
    m_Swapchain.textures.resize(imageCount);
    m_Swapchain.acquiredImage.resize(imageCount);
    m_Swapchain.imageReady.resize(imageCount);
    for (int i = 0; i < imageCount; ++i)
    {
        auto& tex = m_Swapchain.textures[i];

        tex.width = chosenExtent.width;
        tex.height = chosenExtent.height;
        tex.image = images[i];
        tex.alloc = VK_NULL_HANDLE;
        tex.extent = chosenExtent;
        tex.format = chosenFormat.format;
        tex.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        tex.usage = TextureUsage::kPresentSrc;

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
        LC_CHECK(vkCreateImageView(m_Handle, &viewCreateInfo, nullptr, &tex.imageView));

        // Create semaphores
        auto semaphoreInfo = VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };
        LC_CHECK(vkCreateSemaphore(m_Handle, &semaphoreInfo, nullptr, &m_Swapchain.acquiredImage[i]));
        LC_CHECK(vkCreateSemaphore(m_Handle, &semaphoreInfo, nullptr, &m_Swapchain.imageReady[i]));
    }
}

Pipeline* VulkanDevice::CreatePipeline(const PipelineSettings& settings)
{
    auto shader = m_ShaderCache->Compile(settings.shaderName);
    LC_ASSERT(shader);

    auto pipeline = settings.type == PipelineType::kGraphics ?
        CreateGraphicsPipeline(settings, shader) :
        CreateComputePipeline(settings, shader);

    return m_Pipelines.emplace_back(std::move(pipeline)).get();
}

std::unique_ptr<VulkanPipeline> VulkanDevice::CreateGraphicsPipeline(
    const PipelineSettings& settings, Shader* shaderPtr)
{
    LC_ASSERT(settings.framebuffer);

    auto pipelinePtr = std::make_unique<VulkanPipeline>(settings);
    if (!shaderPtr) return pipelinePtr;

    auto& shader = *shaderPtr;
    auto& pipeline = *pipelinePtr;
    auto& framebuffer = *Get(settings.framebuffer);

    pipeline.type = PipelineType::kGraphics;
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

    Array <VkPipelineColorBlendAttachmentState, kMaxColorAttachments> colorAttachments;
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
    LC_CHECK(vkCreateGraphicsPipelines(m_Handle, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline.handle));

    return pipelinePtr;
}

std::unique_ptr<VulkanPipeline> VulkanDevice::CreateComputePipeline(
    const PipelineSettings& settings, Shader* shaderPtr)
{
    auto pipelinePtr = std::make_unique<VulkanPipeline>(settings);
    if (!shaderPtr) return pipelinePtr;

    auto& shader = *shaderPtr;
    auto& pipeline = *pipelinePtr;

    pipeline.type = PipelineType::kCompute;
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
    LC_CHECK(vkCreateComputePipelines(m_Handle, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &pipeline.handle));

    return pipelinePtr;
}

void VulkanDevice::ReloadPipelines()
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

void VulkanDevice::FreePipeline(VulkanPipeline* pipeline)
{
    vkDestroyPipeline(m_Handle, pipeline->handle, nullptr);
}

Buffer* VulkanDevice::CreateBuffer(BufferType type, size_t size)
{
    auto& buff = *m_Buffers.emplace_back(std::make_unique<VulkanBuffer>());

    buff.type = type;
    buff.device = this;
    buff.bufSize = size;

    auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };

    auto bufferInfo = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = BufferTypeToFlags(type),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    LC_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buff.handle, &buff.allocation, nullptr));

    return &buff;
}

Texture* VulkanDevice::CreateTexture(const TextureSettings& info)
{
    auto& tex = *m_Textures.emplace_back(std::make_unique<VulkanTexture>());

    auto format = TextureFormatToVkFormat(info.format);
    auto aspect = TextureFormatToAspect(info.format);
    auto usage = TextureUsageToVkUsage(info.usage);
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

    tex.samples = info.samples;
    tex.texFormat = info.format;
    tex.format = format;
    tex.extent = { .width = info.width, .height = info.height };
    tex.aspect = aspect;
    tex.levels = info.levels;
    tex.usage = info.usage;
    tex.device = this;
    tex.width = info.width;
    tex.height = info.height;
    tex.settings = info;

    if (info.generateMips)
    {
        auto levels = (uint32)Floor(Log2((float)Max(tex.width, tex.height))) + 1;
        tex.levels = levels;
        tex.settings.levels = levels;
    }

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
        .mipLevels = tex.levels,
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

    LC_CHECK(vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &tex.image, &tex.alloc, nullptr));

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
    LC_CHECK(vkCreateImageView(m_Handle, &viewInfo, nullptr, &tex.imageView));

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
            LC_CHECK(vkCreateImageView(m_Handle, &mipInfo, nullptr, &view));

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
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    vkCreateSampler(m_Handle, &samplerInfo, nullptr, &tex.sampler);

    // Transition image to starting layout
    {
        auto startLayout = tex.StartingLayout();

        m_OneShotContext->Begin();

        auto imgBarrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE_KHR,
            .dstAccessMask = VK_ACCESS_NONE_KHR,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = startLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = tex.image,
            .subresourceRange = {
                .aspectMask = tex.aspect,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
        };
        vkCmdPipelineBarrier(m_OneShotContext->m_CommandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imgBarrier);

        m_OneShotContext->End();
        Submit(m_OneShotContext);
    }

    return &tex;
}

Framebuffer* VulkanDevice::CreateFramebuffer(const FramebufferSettings& info)
{
    auto& fb = *m_Framebuffers.emplace_back(std::make_unique<VulkanFramebuffer>());

    fb.colorTextures = info.colorTextures;
    fb.depthTexture = info.depthTexture;

    fb.extent = Get(info.colorTextures.empty() ? info.depthTexture : info.colorTextures.front())->extent;
    fb.samples = Get(info.colorTextures.empty() ? info.depthTexture : info.colorTextures.front())->samples;

    // Create image views if a specific layer or level is requested
    auto createTempView =
        [&](VulkanTexture* texture, int layer, int level, VkImageAspectFlags aspectFlags) -> VkImageView
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
            LC_CHECK(vkCreateImageView(m_Handle, &viewInfo, nullptr, &view));

            return view;
        };

    for (auto texture: fb.colorTextures)
    {
        fb.colorImageViews
            .push_back(createTempView(Get(texture), info.colorLayer, info.colorLevel, VK_IMAGE_ASPECT_COLOR_BIT));
    }
    fb.depthImageView =
        createTempView(Get(info.depthTexture), info.depthLayer, info.depthLevel, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Create render pass

    // Internal convention used here is that all color attachments are placed at indices starting at 0, then the depth
    // attachment (if present) is placed at the end

    Array <VkAttachmentDescription, kMaxAttachments> attachments;
    Array <VkImageView, kMaxAttachments> imageViews;
    auto depthIndex = VK_ATTACHMENT_UNUSED;

    // Populate attachment descriptions
    for (int i = 0; i < fb.colorTextures.size(); ++i)
    {
        auto colorTexture = Get(fb.colorTextures[i]);
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
            .format = Get(info.depthTexture)->format,
            .samples = static_cast<VkSampleCountFlagBits>(Get(info.depthTexture)->samples),
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        });
        imageViews.push_back(fb.depthImageView ? fb.depthImageView : Get(info.depthTexture)->imageView);
    }

    // Populate attachment references
    Array <VkAttachmentReference, kMaxColorAttachments> colorRefs;
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
    LC_CHECK(vkCreateRenderPass(m_Handle, &passInfo, nullptr, &fb.renderPass));

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
    LC_CHECK(vkCreateFramebuffer(m_Handle, &fbInfo, nullptr, &fb.handle));

    return &fb;
}

Context* VulkanDevice::CreateContext()
{
    auto& ctx = *m_Contexts.emplace_back(std::make_unique<VulkanContext>(*this));
    return &ctx;
}

Texture* VulkanDevice::AcquireSwapchainImage()
{
    // Acquire image
    vkAcquireNextImageKHR(m_Handle,
        m_Swapchain.handle,
        UINT64_MAX,
        m_Swapchain.acquiredImage[GetSwapchainIndex()],
        VK_NULL_HANDLE,
        &m_SwapchainImageIndex);

    m_SwapchainImageAcquired = true;

    return &m_Swapchain.textures[m_SwapchainImageIndex];
}

void VulkanDevice::Submit(Context* generalContext)
{
    auto context = Get(generalContext);

    if (m_SwapchainImageAcquired)
    {
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
        auto swapchainImageIndex = GetSwapchainIndex();

        auto submitInfo = VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_Swapchain.acquiredImage[swapchainImageIndex],
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &context->m_CommandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &m_Swapchain.imageReady[swapchainImageIndex]
        };
        LC_CHECK(vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, context->m_ReadyFence));
        m_SwapchainImageAcquired = false;
    }
    else
    {
        auto submitInfo = VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &context->m_CommandBuffer,
        };
        LC_CHECK(vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, context->m_ReadyFence));
    }
}

bool VulkanDevice::Present()
{
    auto presentInfo = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_Swapchain.imageReady[GetSwapchainIndex()],
        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain.handle,
        .pImageIndices = &m_SwapchainImageIndex
    };
    auto presentResult = vkQueuePresentKHR(m_PresentQueue.handle, &presentInfo);

    ++m_FrameIndex;
    return presentResult == VK_SUCCESS;
}

VkBool32 VulkanDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
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

void VulkanDevice::DestroyBuffer(Buffer* buffer)
{

}
void VulkanDevice::DestroyTexture(Texture* texture)
{

}
void VulkanDevice::DestroyPipeline(Pipeline* pipeline)
{

}
void VulkanDevice::DestroyFramebuffer(Framebuffer* framebuffer)
{

}
void VulkanDevice::DestroyContext(Context* context)
{

}
uint32 VulkanDevice::GetSwapchainIndex() const
{
    return m_FrameIndex % m_Swapchain.textures.size();
}

void VulkanDevice::WaitIdle()
{
    vkDeviceWaitIdle(m_Handle);
}

void VulkanDevice::RebuildSwapchain()
{
    DestroySwapchain();
    CreateSwapchain();
}
void VulkanDevice::DestroySwapchain()
{
    for (int i = 0; i < m_Swapchain.textures.size(); ++i)
    {
        vkDestroyImageView(m_Handle, m_Swapchain.textures[i].imageView, nullptr);
        vkDestroySemaphore(m_Handle, m_Swapchain.acquiredImage[i], nullptr);
        vkDestroySemaphore(m_Handle, m_Swapchain.imageReady[i], nullptr);
    }
    vkDestroySwapchainKHR(m_Handle, m_Swapchain.handle, nullptr);
}

}
