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
    case BufferType::UniformDynamic:
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
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    case BufferType::UniformDynamic:
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
    case TextureFormat::kR8:
        return VK_FORMAT_R8_UNORM;

    case TextureFormat::kRGBA8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case TextureFormat::kRGBA8:
        return VK_FORMAT_R8G8B8A8_UNORM;

    case TextureFormat::kRGBA32F:
        return VK_FORMAT_R32G32B32A32_SFLOAT;

    case TextureFormat::kDepth:
        return VK_FORMAT_D32_SFLOAT;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}

static VkImageAspectFlags TextureFormatToAspect(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::kDepth:
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
    case TextureFormat::kRGBA32F:
        // TODO: Narrow this down
        return VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    case TextureFormat::kDepth:
        return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

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

    default:
        LC_ASSERT(0 && "Invalid address mode");
        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
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

    m_Cube.vertices = CreateBuffer(BufferType::Vertex, vertices.size() * sizeof(Vertex));
    m_Cube.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex));

    m_Cube.indices = CreateBuffer(BufferType::Index, indices.size() * sizeof(uint32));
    m_Cube.indices->Upload(indices.data(), indices.size() * sizeof(uint32));
    m_Cube.numIndices = indices.size();
}

void Device::CreateQuad()
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    vertices.assign({
        { .position = { -1.0f, -1.0f, 0.0f }, .texCoord0 = { 0.0f, 1.0f }},
        { .position = { 1.0f, -1.0f, 0.0f }, .texCoord0 = { 1.0f, 1.0f }},
        { .position = { 1.0f, 1.0f, 0.0f }, .texCoord0 = { 1.0f, 0.0f }},
        { .position = { -1.0f, 1.0f, 0.0f }, .texCoord0 = { 0.0f, 0.0f }}
    });

    indices.assign({ 0, 1, 2, 2, 3, 0 });

    m_Quad.vertices = CreateBuffer(BufferType::Vertex, vertices.size() * sizeof(Vertex));
    m_Quad.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex));

    m_Quad.indices = CreateBuffer(BufferType::Index, indices.size() * sizeof(uint32));
    m_Quad.indices->Upload(indices.data(), indices.size() * sizeof(uint32));
    m_Quad.numIndices = indices.size();
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
    m_TransferBuffer = CreateBuffer(BufferType::Staging, 128 * 1024 * 1024);

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

    uint32 green = 0xff00ff00;
    m_GreenTexture = CreateTexture(TextureSettings{}, sizeof(green), &green);

    // Create primitive meshes
    CreateCube();
    CreateQuad();
}

Device::~Device()
{
    vkDeviceWaitIdle(m_Handle);

    // Destroy pipelines
    for (auto& pipeline : m_Pipelines)
    {
        FreePipeline(pipeline.get());
    }

    // Destroy buffers
    for (auto& buff : m_Buffers)
    {
        vmaDestroyBuffer(m_Allocator, buff->handle, buff->allocation);
    }

    // Destroy textures
    for (auto& tex : m_Textures)
    {
        vkDestroyImageView(m_Handle, tex->imageView, nullptr);
        vmaDestroyImage(m_Allocator, tex->image, tex->alloc);
        vkDestroySampler(m_Handle, tex->sampler, nullptr);
    }

    // Destroy contexts
    for (auto& ctx : m_Contexts)
    {
        ctx->Dispose();

    }

    // Destroy shaders
    m_ShaderCache->Clear();

    // Destroy swapchain
    vkDestroyRenderPass(m_Handle, m_Swapchain.framebuffers[0].renderPass, nullptr);
    for (auto& tex : m_Swapchain.textures)
    {
        vkDestroyImageView(m_Handle, tex.imageView, nullptr);
    }
    for (auto& fb : m_Swapchain.framebuffers)
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
    vkDestroyInstance(m_Instance, nullptr);

    glslang::FinalizeProcess();

    glfwTerminate();
}

void Device::CreateInstance()
{
    uint32 instanceExtCount;
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
        .enabledLayerCount = static_cast<uint32>(validationLayers.size()),
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
    uint32 deviceCount;
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
        .queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
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
    uint32 surfaceFormatCount;
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
    result = vkCreateRenderPass(m_Handle, &renderPassInfo, nullptr, &renderPass);
    LC_ASSERT(result == VK_SUCCESS);

    // Create depth texture
    auto depthTexture = CreateTexture(TextureSettings{
        .width = chosenExtent.width,
        .height = chosenExtent.height,
        .format = TextureFormat::kDepth });

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

        fb.extent = chosenExtent;
        fb.renderPass = renderPass;
        fb.usage = FramebufferUsage::SwapchainImage;
        fb.colorTexture = &tex;
        fb.depthTexture = depthTexture;

        // Create framebuffer
        VkImageView fbAttachments[] = { fb.colorTexture->imageView, fb.depthTexture->imageView };

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

Pipeline* Device::CreatePipeline(const PipelineSettings& settings)
{
    auto shader = m_ShaderCache->Compile(settings.shaderName);
    return m_Pipelines.emplace_back(CreatePipeline(settings, shader)).get();
}

std::unique_ptr<Pipeline> Device::CreatePipeline(const PipelineSettings& settings, Shader* shaderPtr)
{
    auto pipelinePtr = std::make_unique<Pipeline>(settings);

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
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, tangent)
        },
        {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, bitangent)
        },
        {
            .location = 4,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = (uint32)offsetof(Vertex, texCoord0)
        },
        {
            .location = 5,
            .binding = 0,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
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
        .x = 0.0f, .y = static_cast<float>(framebuffer.extent.height),
        .width = static_cast<float>(framebuffer.extent.width),
        .height = -static_cast<float>(framebuffer.extent.height),
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
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    auto multisampleInfo = VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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

    auto colorBlendInfo = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentInfo
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

void Device::ReloadPipelines()
{
    vkDeviceWaitIdle(m_Handle);
    for (auto& pipeline : m_Pipelines)
    {
        auto updatedShader = m_ShaderCache->Compile(pipeline->settings.shaderName);
        if (updatedShader)
        {
            if (pipeline->shader != updatedShader)
            {
                auto updatedPipeline = CreatePipeline(pipeline->settings, updatedShader);
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

Texture* Device::CreateTexture(const TextureSettings& info, size_t size, void* data)
{
    auto& tex = *m_Textures.emplace_back(std::make_unique<Texture>());

    auto format = TextureFormatToVkFormat(info.format);
    auto aspect = TextureFormatToAspect(info.format);
    auto usage = TextureFormatToUsage(info.format);
    auto viewType = TextureShapeToViewType(info.shape);
    auto addressMode = TextureAddressModeToVk(info.addressMode);

    VkFlags flags = 0;
    uint32 arrayLayers = 1;

    if (info.shape == TextureShape::kCube)
    {
        arrayLayers = 6;
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        LC_ASSERT(info.width == info.height);
    }

    tex.texFormat = info.format;
    tex.format = format;
    tex.extent = { .width = info.width, .height = info.height };

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
    vkCreateImageView(m_Handle, &viewInfo, nullptr, &tex.imageView);

    // Create sampler
    auto samplerInfo = VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = addressMode,
        .addressModeV = addressMode,
        .addressModeW = addressMode,
        .mipLodBias = 0,
        .anisotropyEnable = VK_FALSE, // TODO: Enable anisotropy
        .maxAnisotropy = m_DeviceProperties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS, // TODO: Change for shadow maps
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
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

    auto& color = *info.colorTexture;
    auto& depth = *info.depthTexture;

    fb.usage = info.usage;
    fb.extent = info.colorTexture->extent;

    // Create render pass
    VkAttachmentDescription attachments[] =
        {
            VkAttachmentDescription{
                .format = color.format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
            VkAttachmentDescription{
                .format = depth.format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            }
        };

    auto colorRef = VkAttachmentReference{
        .attachment = 0,
        .layout = attachments[0].finalLayout
    };
    auto depthRef = VkAttachmentReference{
        .attachment = 1,
        .layout = attachments[1].finalLayout
    };

    auto subpassDesc = VkSubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRef,
        .pDepthStencilAttachment = &depthRef
    };

    auto passInfo = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = LC_ARRAY_SIZE(attachments),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpassDesc
    };

    auto result = vkCreateRenderPass(m_Handle, &passInfo, nullptr, &fb.renderPass);
    LC_ASSERT(result == VK_SUCCESS);

    // Create framebuffer
    VkImageView views[] = { color.imageView, depth.imageView };

    auto fbInfo = VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = fb.renderPass,
        .attachmentCount = LC_ARRAY_SIZE(views),
        .pAttachments = views,
        .width = color.extent.width,
        .height = color.extent.height,
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
    auto colorBarrier = VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_NONE_KHR,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = color.image,
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
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &colorBarrier);

    // Depth
    auto depthBarrier = VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_NONE_KHR,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = depth.image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
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

const Framebuffer& Device::AcquireFramebuffer()
{
    // Acquire image
    vkAcquireNextImageKHR(m_Handle,
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

    auto submitInfo = context->m_SwapchainWritten ?
        VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_ImageAvailable,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &context->m_CommandBuffer,
            .signalSemaphoreCount = 1,
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

// Buffer
void Buffer::Upload(void* data, size_t size, size_t offset) const
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

}