#include "Device.hpp"

#include <set>
#include <algorithm>
#include <iostream>

#include "GLFW/glfw3.h"
#include "shaderc/shaderc.hpp"

#include "glslang/Public/ShaderLang.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/Include/Types.h"
#include "glslang/Include/intermediate.h"

#include "core/Lucent.hpp"

namespace lucent
{

// Device
static VkBufferUsageFlags BufferTypeToFlags(BufferType type)
{
    VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    switch (type)
    {

    case BufferType::Vertex:
    {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        break;
    }
    case BufferType::Index:
    {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        break;
    }
    case BufferType::Uniform:
    {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;
    }
    case BufferType::Staging:
    {
        flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        break;
    }

    }
    return flags;
}

static VkDescriptorType BufferTypeToDescriptorType(BufferType type)
{
    switch (type)
    {
    case BufferType::Uniform:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    default:
        LC_ASSERT(false);
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

static VkFormat TextureFormatToVkFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::RGBA:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case TextureFormat::Depth:
        return VK_FORMAT_D32_SFLOAT;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}

static VkImageAspectFlags TextureFormatToAspect(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::Depth:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static VkImageUsageFlags TextureFormatToUsage(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::RGBA:
        return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    case TextureFormat::Depth:
        return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    default:
        return 0;
    }
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

    CreateInstance();
    glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface);
    CreateDevice();

    // Initialize VMA
    auto allocatorInfo = VmaAllocatorCreateInfo{
        .physicalDevice = m_PhysicalDevice,
        .device = m_Device,
        .instance = m_Instance,
        .vulkanApiVersion = VK_API_VERSION_1_2
    };
    vmaCreateAllocator(&allocatorInfo, &m_Allocator);

    // Create semaphores and fences
    auto semaphoreInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailable);
    vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinished);

    // Create transfer buffer
    m_TransferBuffer = CreateBuffer(BufferType::Staging, 128 * 1024 * 1024);

    auto poolInfo = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = m_GraphicsQueue.familyIndex
    };
    auto result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_OneShotCmdPool);
    LC_ASSERT(result == VK_SUCCESS);

    // Create descriptor pool
    VkDescriptorPoolSize descPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1024
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1024
        }
    };

    auto descPoolInfo = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1024,
        .poolSizeCount = LC_ARRAY_SIZE(descPoolSizes),
        .pPoolSizes = descPoolSizes
    };

    result = vkCreateDescriptorPool(m_Device, &descPoolInfo, nullptr, &m_DescPool);
    LC_ASSERT(result == VK_SUCCESS);

    CreateSwapchain();
}

Device::~Device()
{
    vkDeviceWaitIdle(m_Device);

    // Destroy pipelines
    for (auto& pipeline : m_Pipelines)
    {
        vkDestroyPipeline(m_Device, pipeline->handle, nullptr);
        vkDestroyDescriptorSetLayout(m_Device, pipeline->setLayouts[0], nullptr);
        vkDestroyDescriptorSetLayout(m_Device, pipeline->setLayouts[1], nullptr);
        vkDestroyPipelineLayout(m_Device, pipeline->layout, nullptr);
    }

    // Destroy buffers
    for (auto& buff : m_Buffers)
    {
        vmaDestroyBuffer(m_Allocator, buff->handle, buff->allocation);
    }

    // Destroy textures
    for (auto& tex : m_Textures)
    {
        vkDestroyImageView(m_Device, tex->imageView, nullptr);
        vmaDestroyImage(m_Allocator, tex->image, tex->alloc);
        vkDestroySampler(m_Device, tex->sampler, nullptr);
    }

    // Destroy contexts
    for (auto& ctx : m_Contexts)
    {
        vkFreeCommandBuffers(m_Device, ctx->m_CommandPool, 1, &ctx->m_CommandBuffer);
        vkDestroyCommandPool(m_Device, ctx->m_CommandPool, nullptr);
    }

    vkDestroyDescriptorPool(m_Device, m_DescPool, nullptr);

    // Destroy swapchain
    vkDestroyRenderPass(m_Device, m_Swapchain.framebuffers[0].renderPass, nullptr);
    for (auto& fb : m_Swapchain.framebuffers)
    {
        vkDestroyImageView(m_Device, fb.imageView, nullptr);
        vkDestroyFramebuffer(m_Device, fb.handle, nullptr);
    }
    vkDestroySwapchainKHR(m_Device, m_Swapchain.handle, nullptr);

    // Destroy semaphores
    vkDestroySemaphore(m_Device, m_ImageAvailable, nullptr);
    vkDestroySemaphore(m_Device, m_RenderFinished, nullptr);

    // Destroy one shot command pool
    vkDestroyCommandPool(m_Device, m_OneShotCmdPool, nullptr);

    // VMA
    vmaDestroyAllocator(m_Allocator);

    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);

    glslang::FinalizeProcess();

    glfwTerminate();
}

void Device::CreateInstance()
{
    uint32_t instanceExtCount;
    const char** instanceExts = glfwGetRequiredInstanceExtensions(&instanceExtCount);

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
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = instanceExtCount,
        .ppEnabledExtensionNames = instanceExts
    };

    auto result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
    LC_ASSERT(result == VK_SUCCESS);
}

void Device::CreateDevice()
{
    // Find suitable physical device
    uint32_t deviceCount;
    std::vector<VkPhysicalDevice> physicalDevices;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    physicalDevices.resize(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, physicalDevices.data());

    VkPhysicalDevice selectedDevice;
    for (auto& device : physicalDevices)
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
    uint32_t queueFamilyCount;
    std::vector<VkQueueFamilyProperties> familyProperties;
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, nullptr);
    familyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, familyProperties.data());

    uint32_t graphicsFamilyIdx = -1;
    uint32_t presentFamilyIdx = -1;
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
    std::set<uint32_t> familyIndices = { graphicsFamilyIdx, presentFamilyIdx };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(familyIndices.size());
    for (auto idx : familyIndices)
    {
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = idx,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    // Create device
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    auto deviceCreateInfo = VkDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    auto result = vkCreateDevice(selectedDevice, &deviceCreateInfo, nullptr, &m_Device);
    LC_ASSERT(result == VK_SUCCESS);

    m_GraphicsQueue.familyIndex = graphicsFamilyIdx;
    m_PresentQueue.familyIndex = presentFamilyIdx;

    vkGetDeviceQueue(m_Device, graphicsFamilyIdx, 0, &m_GraphicsQueue.handle);
    vkGetDeviceQueue(m_Device, presentFamilyIdx, 0, &m_PresentQueue.handle);
}

void Device::CreateSwapchain()
{
    // Find suitable format, extent and present mode
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities);

    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, nullptr);
    surfaceFormats.resize(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, surfaceFormats.data());

    VkSurfaceFormatKHR chosenFormat;
    for (auto& format : surfaceFormats)
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

        chosenExtent = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

        auto& min = surfaceCapabilities.minImageExtent;
        auto& max = surfaceCapabilities.maxImageExtent;

        chosenExtent.width = std::clamp(chosenExtent.width, min.width, max.width);
        chosenExtent.height = std::clamp(chosenExtent.height, min.height, max.height);
    }

    auto chosenImageCount = surfaceCapabilities.minImageCount + 1;

    auto sharedQueue = m_GraphicsQueue.familyIndex == m_PresentQueue.familyIndex;
    uint32_t queueIndices[] = { m_GraphicsQueue.familyIndex, m_PresentQueue.familyIndex };

    auto swapchainCreateInfo = VkSwapchainCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_Surface,
        .minImageCount = chosenImageCount,
        .imageFormat = chosenFormat.format,
        .imageColorSpace = chosenFormat.colorSpace,
        .imageExtent = chosenExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = sharedQueue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = sharedQueue ? 0u : 2u,
        .pQueueFamilyIndices = sharedQueue ? nullptr : queueIndices,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = chosenPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    auto result = vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, nullptr, &m_Swapchain.handle);
    LC_ASSERT(result == VK_SUCCESS);

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain.handle, &imageCount, nullptr);
    m_Swapchain.framebuffers.resize(imageCount);

    std::vector<VkImage> images;
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain.handle, &imageCount, images.data());

    // Create render pass
    auto colAttachmentDescription = VkAttachmentDescription{
        .format = chosenFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    auto depthAttachmentDescription = VkAttachmentDescription{
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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

    auto dependency = VkSubpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    VkAttachmentDescription attachments[] = { colAttachmentDescription, depthAttachmentDescription };

    auto renderPassInfo = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpassDesc,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkRenderPass renderPass;
    result = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPass);
    LC_ASSERT(result == VK_SUCCESS);

    // Create depth texture
    auto depthTexture = CreateTexture(TextureInfo{
        .width = chosenExtent.width,
        .height = chosenExtent.height,
        .format = TextureFormat::Depth });

    // Initialize swapchain framebuffers
    for (int i = 0; i < m_Swapchain.framebuffers.size(); ++i)
    {
        auto& fb = m_Swapchain.framebuffers[i];

        fb.renderPass = renderPass;
        fb.extent = chosenExtent;
        fb.format = chosenFormat.format;
        fb.image = images[i];
        fb.depthTexture = depthTexture;

        // Create image view
        auto viewCreateInfo = VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = fb.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = fb.format,
            .components = VkComponentMapping{},
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        result = vkCreateImageView(m_Device, &viewCreateInfo, nullptr, &fb.imageView);
        LC_ASSERT(result == VK_SUCCESS);

        // Create framebuffer
        VkImageView fbAttachments[] = { fb.imageView, fb.depthTexture->imageView };

        auto framebufferInfo = VkFramebufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = fb.renderPass,
            .attachmentCount = LC_ARRAY_SIZE(fbAttachments),
            .pAttachments = fbAttachments,
            .width = fb.extent.width,
            .height = fb.extent.height,
            .layers = 1
        };
        result = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &fb.handle);
        LC_ASSERT(result == VK_SUCCESS);
    }
}

void Device::CreateShader(const std::string& vertText, const std::string& fragText)
{
    TBuiltInResource res = {
        /*.maxLights = */ 8,         // From OpenGL 3.0 table 6.46.
        /*.maxClipPlanes = */ 6,     // From OpenGL 3.0 table 6.46.
        /*.maxTextureUnits = */ 2,   // From OpenGL 3.0 table 6.50.
        /*.maxTextureCoords = */ 8,  // From OpenGL 3.0 table 6.50.
        /*.maxVertexAttribs = */ 16,
        /*.maxVertexUniformComponents = */ 4096,
        /*.maxVaryingFloats = */ 60,  // From OpenGLES 3.1 table 6.44.
        /*.maxVertexTextureImageUnits = */ 16,
        /*.maxCombinedTextureImageUnits = */ 80,
        /*.maxTextureImageUnits = */ 16,
        /*.maxFragmentUniformComponents = */ 1024,

        // glslang has 32 maxDrawBuffers.
        // Pixel phone Vulkan driver in Android N has 8
        // maxFragmentOutputAttachments.
        /*.maxDrawBuffers = */ 8,

        /*.maxVertexUniformVectors = */ 256,
        /*.maxVaryingVectors = */ 15,  // From OpenGLES 3.1 table 6.44.
        /*.maxFragmentUniformVectors = */ 256,
        /*.maxVertexOutputVectors = */ 16,   // maxVertexOutputComponents / 4
        /*.maxFragmentInputVectors = */ 15,  // maxFragmentInputComponents / 4
        /*.minProgramTexelOffset = */ -8,
        /*.maxProgramTexelOffset = */ 7,
        /*.maxClipDistances = */ 8,
        /*.maxComputeWorkGroupCountX = */ 65535,
        /*.maxComputeWorkGroupCountY = */ 65535,
        /*.maxComputeWorkGroupCountZ = */ 65535,
        /*.maxComputeWorkGroupSizeX = */ 1024,
        /*.maxComputeWorkGroupSizeX = */ 1024,
        /*.maxComputeWorkGroupSizeZ = */ 64,
        /*.maxComputeUniformComponents = */ 512,
        /*.maxComputeTextureImageUnits = */ 16,
        /*.maxComputeImageUniforms = */ 8,
        /*.maxComputeAtomicCounters = */ 8,
        /*.maxComputeAtomicCounterBuffers = */ 1,  // From OpenGLES 3.1 Table 6.43
        /*.maxVaryingComponents = */ 60,
        /*.maxVertexOutputComponents = */ 64,
        /*.maxGeometryInputComponents = */ 64,
        /*.maxGeometryOutputComponents = */ 128,
        /*.maxFragmentInputComponents = */ 128,
        /*.maxImageUnits = */ 8,  // This does not seem to be defined anywhere,
        // set to ImageUnits.
        /*.maxCombinedImageUnitsAndFragmentOutputs = */ 8,
        /*.maxCombinedShaderOutputResources = */ 8,
        /*.maxImageSamples = */ 0,
        /*.maxVertexImageUniforms = */ 0,
        /*.maxTessControlImageUniforms = */ 0,
        /*.maxTessEvaluationImageUniforms = */ 0,
        /*.maxGeometryImageUniforms = */ 0,
        /*.maxFragmentImageUniforms = */ 8,
        /*.maxCombinedImageUniforms = */ 8,
        /*.maxGeometryTextureImageUnits = */ 16,
        /*.maxGeometryOutputVertices = */ 256,
        /*.maxGeometryTotalOutputComponents = */ 1024,
        /*.maxGeometryUniformComponents = */ 512,
        /*.maxGeometryVaryingComponents = */ 60,  // Does not seem to be defined
        // anywhere, set equal to
        // maxVaryingComponents.
        /*.maxTessControlInputComponents = */ 128,
        /*.maxTessControlOutputComponents = */ 128,
        /*.maxTessControlTextureImageUnits = */ 16,
        /*.maxTessControlUniformComponents = */ 1024,
        /*.maxTessControlTotalOutputComponents = */ 4096,
        /*.maxTessEvaluationInputComponents = */ 128,
        /*.maxTessEvaluationOutputComponents = */ 128,
        /*.maxTessEvaluationTextureImageUnits = */ 16,
        /*.maxTessEvaluationUniformComponents = */ 1024,
        /*.maxTessPatchComponents = */ 120,
        /*.maxPatchVertices = */ 32,
        /*.maxTessGenLevel = */ 64,
        /*.maxViewports = */ 16,
        /*.maxVertexAtomicCounters = */ 0,
        /*.maxTessControlAtomicCounters = */ 0,
        /*.maxTessEvaluationAtomicCounters = */ 0,
        /*.maxGeometryAtomicCounters = */ 0,
        /*.maxFragmentAtomicCounters = */ 8,
        /*.maxCombinedAtomicCounters = */ 8,
        /*.maxAtomicCounterBindings = */ 1,
        /*.maxVertexAtomicCounterBuffers = */ 0,  // From OpenGLES 3.1 Table 6.41.

        // ARB_shader_atomic_counters.
        /*.maxTessControlAtomicCounterBuffers = */ 0,
        /*.maxTessEvaluationAtomicCounterBuffers = */ 0,
        /*.maxGeometryAtomicCounterBuffers = */ 0,
        // /ARB_shader_atomic_counters.

        /*.maxFragmentAtomicCounterBuffers = */ 0,  // From OpenGLES 3.1 Table 6.43.
        /*.maxCombinedAtomicCounterBuffers = */ 1,
        /*.maxAtomicCounterBufferSize = */ 32,
        /*.maxTransformFeedbackBuffers = */ 4,
        /*.maxTransformFeedbackInterleavedComponents = */ 64,
        /*.maxCullDistances = */ 8,                 // ARB_cull_distance.
        /*.maxCombinedClipAndCullDistances = */ 8,  // ARB_cull_distance.
        /*.maxSamples = */ 4,
        /* .maxMeshOutputVerticesNV = */ 256,
        /* .maxMeshOutputPrimitivesNV = */ 512,
        /* .maxMeshWorkGroupSizeX_NV = */ 32,
        /* .maxMeshWorkGroupSizeY_NV = */ 1,
        /* .maxMeshWorkGroupSizeZ_NV = */ 1,
        /* .maxTaskWorkGroupSizeX_NV = */ 32,
        /* .maxTaskWorkGroupSizeY_NV = */ 1,
        /* .maxTaskWorkGroupSizeZ_NV = */ 1,
        /* .maxMeshViewCountNV = */ 4,
        /* .maxDualSourceDrawBuffersEXT = */ 1,
        // This is the glslang TLimits structure.
        // It defines whether or not the following features are enabled.
        // We want them to all be enabled.
        /*.limits = */ {
            /*.nonInductiveForLoops = */ 1,
            /*.whileLoops = */ 1,
            /*.doWhileLoops = */ 1,
            /*.generalUniformIndexing = */ 1,
            /*.generalAttributeMatrixVectorIndexing = */ 1,
            /*.generalVaryingIndexing = */ 1,
            /*.generalSamplerIndexing = */ 1,
            /*.generalVariableIndexing = */ 1,
            /*.generalConstantMatrixVectorIndexing = */ 1,
        }};

    glslang::TShader vertShader(EShLangVertex);
    auto vertStr = vertText.c_str();
    vertShader.setStrings(&vertStr, 1);
    vertShader.parse(&res, 450, true, EShMsgDefault);

    glslang::TShader fragShader(EShLangFragment);
    auto fragStr = fragText.c_str();
    fragShader.setStrings(&fragStr, 1);
    fragShader.parse(&res, 450, true, EShMsgDefault);

    glslang::TProgram program;
    program.addShader(&vertShader);
    program.addShader(&fragShader);
    program.link(EShMsgDefault);

    auto vInter = fragShader.getIntermediate();
    auto root = vInter->getTreeRoot();

    for (auto node : root->getAsAggregate()->getSequence())
    {
        auto agg = node->getAsAggregate();
        if (agg->getOp() == glslang::EOpLinkerObjects)
        {
            for (auto obj : agg->getSequence())
            {
                auto symbol = obj->getAsSymbolNode();
                auto& type = symbol->getType();
                auto& qualifier = type.getQualifier();

                if (qualifier.hasUniformLayout())
                {
                    std::cout << "Name: " << symbol->getName() << "\n"
                              << "Set: " << qualifier.layoutSet << "\n"
                              << "Binding: " << qualifier.layoutBinding << "\n" << std::endl;
                }
            }
        }
    }

}

Pipeline* Device::CreatePipeline(const PipelineInfo& info)
{
    auto& pipe = *m_Pipelines.emplace_back(std::make_unique<Pipeline>());

    CreateShader(info.vertexShader, info.fragmentShader);

    // Create shader modules
    shaderc::Compiler compiler;
    auto vertResult = compiler.CompileGlslToSpv(info.vertexShader, shaderc_vertex_shader, "vert.glsl");
    auto fragResult = compiler.CompileGlslToSpv(info.fragmentShader, shaderc_fragment_shader, "frag.glsl");

    auto vertModuleInfo = VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<size_t>(std::distance(vertResult.begin(), vertResult.end())
            * sizeof(*vertResult.begin())),
        .pCode = vertResult.begin()
    };

    auto fragModuleInfo = VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<size_t>(std::distance(fragResult.begin(), fragResult.end())
            * sizeof(*fragResult.begin())),
        .pCode = fragResult.begin()
    };

    VkShaderModule vertModule, fragModule;

    auto result = vkCreateShaderModule(m_Device, &vertModuleInfo, nullptr, &vertModule);
    LC_ASSERT(result == VK_SUCCESS);

    result = vkCreateShaderModule(m_Device, &fragModuleInfo, nullptr, &fragModule);
    LC_ASSERT(result == VK_SUCCESS);

    // Create shader stages
    auto vertStageInfo = VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertModule,
        .pName = "main"
    };

    auto fragStageInfo = VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragModule,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo stages[] = { vertStageInfo, fragStageInfo };

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
            .offset = (uint32_t)offsetof(Vertex, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32_t)offsetof(Vertex, normal)
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32_t)offsetof(Vertex, tangent)
        },
        {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = (uint32_t)offsetof(Vertex, texCoord0)
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
        .width = static_cast<float>(m_Swapchain.framebuffers[0].extent.width),
        .height = static_cast<float>(m_Swapchain.framebuffers[0].extent.height),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };

    auto scissor = VkRect2D{
        .offset = {},
        .extent = m_Swapchain.framebuffers[0].extent
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
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    auto multisampleInfo = VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    auto depthStencilInfo = VkPipelineDepthStencilStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    auto colorBlendAttachmentInfo = VkPipelineColorBlendAttachmentState{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT
    };

    auto colorBlendInfo = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentInfo
    };

    // TODO: CHANGE
    // Create pipeline layout & descriptor layout
    VkDescriptorSetLayoutBinding descBindings0[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL
        }
    };

    VkDescriptorSetLayoutBinding descBindings1[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL
        }
    };

    auto descSetInfo0 = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = LC_ARRAY_SIZE(descBindings0),
        .pBindings = descBindings0
    };
    result = vkCreateDescriptorSetLayout(m_Device, &descSetInfo0, nullptr, &pipe.setLayouts[0]);
    LC_ASSERT(result == VK_SUCCESS);

    auto descSetInfo1 = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = LC_ARRAY_SIZE(descBindings1),
        .pBindings = descBindings1
    };
    result = vkCreateDescriptorSetLayout(m_Device, &descSetInfo1, nullptr, &pipe.setLayouts[1]);
    LC_ASSERT(result == VK_SUCCESS);


    auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 2,
        .pSetLayouts = pipe.setLayouts
    };
    result = vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &pipe.layout);
    LC_ASSERT(result == VK_SUCCESS);

    auto pipelineCreateInfo = VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizationInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = &depthStencilInfo,
        .pColorBlendState = &colorBlendInfo,
        .layout = pipe.layout,
        .renderPass = m_Swapchain.framebuffers[0].renderPass,
        .subpass = 0
    };

    result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipe.handle);
    LC_ASSERT(result == VK_SUCCESS);

    // Destroy shader modules
    vkDestroyShaderModule(m_Device, vertModule, nullptr);
    vkDestroyShaderModule(m_Device, fragModule, nullptr);

    return &pipe;
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

Texture* Device::CreateTexture(const TextureInfo& info, size_t size, void* data)
{
    auto& tex = *m_Textures.emplace_back(std::make_unique<Texture>());
    tex.format = info.format;

    auto format = TextureFormatToVkFormat(info.format);
    auto aspect = TextureFormatToAspect(info.format);
    auto usage = TextureFormatToUsage(info.format);

    // Create image
    auto imageInfo = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            .width = info.width,
            .height = info.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
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
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {},
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vkCreateImageView(m_Device, &viewInfo, nullptr, &tex.imageView);

    // Create sampler
    auto samplerInfo = VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0,
        .anisotropyEnable = VK_FALSE, // TODO: Enable anisotropy
        .maxAnisotropy = m_DeviceProperties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    vkCreateSampler(m_Device, &samplerInfo, nullptr, &tex.sampler);

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
        vkAllocateCommandBuffers(m_Device, &cbAllocInfo, &cb);

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
        vkFreeCommandBuffers(m_Device, m_OneShotCmdPool, 1, &cb);
        vkResetCommandPool(m_Device, m_OneShotCmdPool, 0);
    }

    return &tex;
}

Context* Device::CreateContext()
{
    auto& ctx = *m_Contexts.emplace_back(std::make_unique<Context>(*this));

    // Create command pool
    auto poolCreateInfo = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = m_GraphicsQueue.familyIndex
    };

    auto result = vkCreateCommandPool(m_Device, &poolCreateInfo, nullptr, &ctx.m_CommandPool);
    LC_ASSERT(result == VK_SUCCESS);

    auto bufferAllocInfo = VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ctx.m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1u
    };

    result = vkAllocateCommandBuffers(m_Device, &bufferAllocInfo, &ctx.m_CommandBuffer);
    LC_ASSERT(result == VK_SUCCESS);

    return &ctx;
}

const Framebuffer& Device::AcquireFramebuffer()
{
    // Acquire image
    vkAcquireNextImageKHR(m_Device,
        m_Swapchain.handle,
        UINT64_MAX,
        m_ImageAvailable,
        VK_NULL_HANDLE,
        &m_NextImageIndex);

    return m_Swapchain.framebuffers[m_NextImageIndex];
}

void Device::Submit(Context* context)
{
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    auto submitInfo = VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_ImageAvailable,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &context->m_CommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_RenderFinished
    };

    auto result = vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
    LC_ASSERT(result == VK_SUCCESS);
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
    vkDeviceWaitIdle(m_Device);
}

// Descriptor sets (temp)
DescriptorSet* Device::CreateDescriptorSet(const Pipeline& pipeline, uint32_t index)
{
    auto& set = m_DescSets.emplace_back(std::make_unique<DescriptorSet>());
    set->index = index;

    auto setAllocInfo = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_DescPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &pipeline.setLayouts[index]
    };

    auto result = vkAllocateDescriptorSets(m_Device, &setAllocInfo, &set->handle);
    LC_ASSERT(result == VK_SUCCESS);

    return set.get();
}

void Device::WriteSet(DescriptorSet* set, uint32_t binding, const Buffer& buffer)
{
    auto buffInfo = VkDescriptorBufferInfo{
        .buffer = buffer.handle,
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    auto descWrite = VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set->handle,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = BufferTypeToDescriptorType(buffer.type),
        .pBufferInfo = &buffInfo
    };

    vkUpdateDescriptorSets(m_Device, 1, &descWrite, 0, nullptr);
}

void Device::WriteSet(DescriptorSet* set, uint32_t binding, const Texture& texture)
{
    auto imgInfo = VkDescriptorImageInfo{
        .sampler = texture.sampler,
        .imageView = texture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    auto descWrite = VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set->handle,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imgInfo
    };

    vkUpdateDescriptorSets(m_Device, 1, &descWrite, 0, nullptr);
}

void Context::BindSet(const DescriptorSet* set)
{
    LC_ASSERT(m_BoundPipeline != nullptr);

    vkCmdBindDescriptorSets(m_CommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_BoundPipeline->layout,
        set->index,
        1,
        &set->handle,
        0,
        nullptr);
}

void Context::BindSet(const DescriptorSet* set, uint32_t dynamicOffset)
{
    LC_ASSERT(m_BoundPipeline != nullptr);

    vkCmdBindDescriptorSets(m_CommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_BoundPipeline->layout,
        set->index,
        1,
        &set->handle,
        1,
        &dynamicOffset);
}

// Buffer
void Buffer::Upload(void* data, size_t size, size_t offset) const
{
    LC_ASSERT(offset + size <= bufSize);

    // TODO: Use staging buffer
    uint8_t* ptr;
    vmaMapMemory(device->m_Allocator, allocation, (void**)&ptr);
    ptr += offset;
    memcpy(ptr, data, size);
    vmaUnmapMemory(device->m_Allocator, allocation);
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

// Context
void Context::Begin() const
{
    vkResetCommandPool(m_Device.m_Device, m_CommandPool, 0);

    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    auto result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    LC_ASSERT(result == VK_SUCCESS);
}

void Context::End() const
{
    auto result = vkEndCommandBuffer(m_CommandBuffer);
    LC_ASSERT(result == VK_SUCCESS);
}

void Context::BeginRenderPass(const Framebuffer& fbuffer) const
{
    VkClearValue clearValues[] = {
        { .color = { 0.0f, 0.0f, 0.0f, 1.0f }},
        { .depthStencil = { .depth = 1.0f, .stencil = 0 }}
    };

    auto renderPassBeginInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = fbuffer.renderPass,
        .framebuffer = fbuffer.handle,
        .renderArea = {
            .offset = {},
            .extent = fbuffer.extent
        },
        .clearValueCount = LC_ARRAY_SIZE(clearValues),
        .pClearValues = clearValues
    };
    vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Context::EndRenderPass() const
{
    vkCmdEndRenderPass(m_CommandBuffer);
}

void Context::Bind(Pipeline& pipeline)
{
    vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
    m_BoundPipeline = &pipeline;
}

void Context::Bind(const Buffer* indexBuffer)
{
    vkCmdBindIndexBuffer(m_CommandBuffer, indexBuffer->handle, 0, VK_INDEX_TYPE_UINT32);
}

void Context::Bind(const Buffer* vertexBuffer, uint32_t binding)
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, &vertexBuffer->handle, &offset);
}

void Context::Draw(uint32_t indexCount) const
{
    vkCmdDrawIndexed(m_CommandBuffer, indexCount, 1, 0, 0, 0);
}

}