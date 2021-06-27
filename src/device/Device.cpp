#include "Device.hpp"

#include <set>
#include <algorithm>

#include "GLFW/glfw3.h"
#include "shaderc/shaderc.hpp"

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
    }

    return flags;
}

static VkDescriptorType BufferTypeToDescriptorType(BufferType type)
{
    switch (type)
    {
    case BufferType::Uniform:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    default:
        LC_ASSERT(false);
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

Device::Device()
{
    // Set up GLFW
    if (!glfwInit())
        return;

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

    CreateSwapchain();

    // Create semaphores and fences
    auto semaphoreInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailable);
    vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinished);
}

Device::~Device()
{
    vkDeviceWaitIdle(m_Device);

    // Destroy pipelines
    for (auto& pipeline : m_Pipelines)
    {
        vkDestroyPipeline(m_Device, pipeline->handle, nullptr);
        vkDestroyDescriptorSetLayout(m_Device, pipeline->setLayouts[0], nullptr);
        vkDestroyPipelineLayout(m_Device, pipeline->layout, nullptr);
    }

    // Destroy buffers
    for (auto& buff : m_Buffers)
    {
        vmaDestroyBuffer(m_Allocator, buff->handle, buff->allocation);
    }

    // Destroy contexts
    for (auto& ctx : m_Contexts)
    {
        vkFreeCommandBuffers(m_Device, ctx->m_CommandPool, 1, &ctx->m_CommandBuffer);
        vkDestroyCommandPool(m_Device, ctx->m_CommandPool, nullptr);

        vkDestroyDescriptorPool(m_Device, ctx->m_DescPool, nullptr);
    }

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

    // VMA
    vmaDestroyAllocator(m_Allocator);

    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);

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
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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
    auto attachmentDescription = VkAttachmentDescription{
        .format = chosenFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    auto attachmentReference = VkAttachmentReference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    auto subpassDesc = VkSubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentReference
    };

    auto dependency = VkSubpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    auto renderPassInfo = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpassDesc,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkRenderPass renderPass;
    result = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPass);
    LC_ASSERT(result == VK_SUCCESS);

    // Initialize swapchain framebuffers
    for (int i = 0; i < m_Swapchain.framebuffers.size(); ++i)
    {
        auto& fb = m_Swapchain.framebuffers[i];

        fb.renderPass = renderPass;
        fb.extent = chosenExtent;
        fb.format = chosenFormat.format;
        fb.image = images[i];

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
        auto framebufferInfo = VkFramebufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = fb.renderPass,
            .attachmentCount = 1,
            .pAttachments = &fb.imageView,
            .width = fb.extent.width,
            .height = fb.extent.height,
            .layers = 1
        };
        result = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &fb.handle);
        LC_ASSERT(result == VK_SUCCESS);
    }
}

Pipeline* Device::CreatePipeline(const PipelineInfo& info)
{
    auto& pipe = *m_Pipelines.emplace_back(std::make_unique<Pipeline>());

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
    auto vertexBindingInfo = VkVertexInputBindingDescription{
        .binding = 0,
        .stride = 3 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    auto vertexAttributeInfo = VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
    };

    auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBindingInfo,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &vertexAttributeInfo
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
    auto descBinding = VkDescriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL
    };

    auto descSetInfo = VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &descBinding
    };
    result = vkCreateDescriptorSetLayout(m_Device, &descSetInfo, nullptr, &pipe.setLayouts[0]);
    LC_ASSERT(result == VK_SUCCESS);

    auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &pipe.setLayouts[0]
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

Context* Device::CreateContext()
{
    auto& ctx = *m_Contexts.emplace_back(std::make_unique<Context>(*this));

    // Create command pool
    auto poolCreateInfo = VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
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

    auto beginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    result = vkBeginCommandBuffer(ctx.m_CommandBuffer, &beginInfo);
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

    result = vkCreateDescriptorPool(m_Device, &descPoolInfo, nullptr, &ctx.m_DescPool);
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
    auto result = vkEndCommandBuffer(context->m_CommandBuffer);
    LC_ASSERT(result == VK_SUCCESS);

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

    result = vkQueueSubmit(m_GraphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
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
    vkQueueWaitIdle(m_PresentQueue.handle);
}

// Buffer

void Buffer::Upload(void* data, size_t size) const
{
    // TODO: Use staging buffer
    void* ptr;
    vmaMapMemory(device->m_Allocator, allocation, &ptr);
    memcpy(ptr, data, size);
    vmaUnmapMemory(device->m_Allocator, allocation);
    vmaFlushAllocation(device->m_Allocator, allocation, 0, VK_WHOLE_SIZE);
}

// Context

void Context::BeginRenderPass(const Framebuffer& fbuffer) const
{
    VkClearValue clearCol = { 0.0f, 0.0f, 0.0f, 1.0f };

    auto renderPassBeginInfo = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = fbuffer.renderPass,
        .framebuffer = fbuffer.handle,
        .renderArea = {
            .offset = {},
            .extent = fbuffer.extent
        },
        .clearValueCount = 1,
        .pClearValues = &clearCol
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

void Context::Bind(const Buffer& vertexBuffer, const Buffer& indexBuffer) const
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vertexBuffer.handle, &offset);
    vkCmdBindIndexBuffer(m_CommandBuffer, indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
}

void Context::Draw(uint32_t indexCount) const
{
    vkCmdDrawIndexed(m_CommandBuffer, indexCount, 1, 0, 0, 0);
}

// Descriptor sets (temp)
DescriptorSet* Context::CreateDescriptorSet(const Pipeline& pipeline, uint32_t index)
{
    auto& set = m_DescSets.emplace_back(std::make_unique<DescriptorSet>());
    set->index = index;

    auto setAllocInfo = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_DescPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &pipeline.setLayouts[index]
    };

    auto result = vkAllocateDescriptorSets(m_Device.m_Device, &setAllocInfo, &set->handle);
    LC_ASSERT(result == VK_SUCCESS);

    return set.get();
}

void Context::WriteSet(DescriptorSet* set, uint32_t binding, const Buffer& buffer)
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

    vkUpdateDescriptorSets(m_Device.m_Device, 1, &descWrite, 0, nullptr);
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

}