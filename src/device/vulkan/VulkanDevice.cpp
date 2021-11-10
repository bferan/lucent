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

namespace lucent
{

VulkanDevice::VulkanDevice(GLFWwindow* window)
    : m_Window(window)
{
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

    m_Swapchain = std::make_unique<VulkanSwapchain>(this);

    InitGeometry(this);
}

VulkanDevice::~VulkanDevice()
{
    vkDeviceWaitIdle(m_Handle);

    m_Contexts.clear();
    m_Pipelines.clear();
    m_Framebuffers.clear();
    m_Textures.clear();
    m_Buffers.clear();
    m_ShaderCache->Clear();

    m_Swapchain.reset();

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

Pipeline* VulkanDevice::CreatePipeline(const PipelineSettings& settings)
{
    auto shader = m_ShaderCache->Compile(settings);
    LC_ASSERT(shader);

    return m_Pipelines.emplace_back(std::make_unique<VulkanPipeline>(this, shader, settings)).get();
}

void VulkanDevice::DestroyPipeline(Pipeline* pipeline)
{
    RemoveResource(pipeline, m_Pipelines);
}

void VulkanDevice::ReloadPipelines()
{
    vkDeviceWaitIdle(m_Handle);
    for (auto& pipeline: m_Pipelines)
    {
        auto updatedShader = m_ShaderCache->Compile(pipeline->GetSettings());
        if (updatedShader)
        {
            if (pipeline->shader != updatedShader)
            {
                *pipeline = VulkanPipeline(this, updatedShader, pipeline->GetSettings());
            }
        }
    }
}

Buffer* VulkanDevice::CreateBuffer(BufferType type, size_t size)
{
    return m_Buffers.emplace_back(std::make_unique<VulkanBuffer>(this, type, size)).get();
}

void VulkanDevice::DestroyBuffer(Buffer* buffer)
{
    RemoveResource(buffer, m_Buffers);
}

Texture* VulkanDevice::CreateTexture(const TextureSettings& settings)
{
    return m_Textures.emplace_back(std::make_unique<VulkanTexture>(this, settings)).get();
}

void VulkanDevice::DestroyTexture(Texture* texture)
{
    RemoveResource(texture, m_Textures);
}

Framebuffer* VulkanDevice::CreateFramebuffer(const FramebufferSettings& info)
{
    return m_Framebuffers.emplace_back(std::make_unique<VulkanFramebuffer>(this, info)).get();
}

void VulkanDevice::DestroyFramebuffer(Framebuffer* framebuffer)
{
    RemoveResource(framebuffer, m_Framebuffers);
}

Context* VulkanDevice::CreateContext()
{
    auto& ctx = *m_Contexts.emplace_back(std::make_unique<VulkanContext>(*this));
    return &ctx;
}

void VulkanDevice::DestroyContext(Context* context)
{
    RemoveResource(context, m_Contexts);
}

Texture* VulkanDevice::AcquireSwapchainImage()
{
    m_SwapchainImageAcquired = true;
    return m_Swapchain->AcquireImage(m_FrameIndex);
}

void VulkanDevice::Submit(Context* generalContext)
{
    auto context = Get(generalContext);

    if (m_SwapchainImageAcquired)
    {
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

        auto submitInfo = VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &context->m_CommandBuffer,
        };
        m_Swapchain->SyncSubmit(m_FrameIndex, submitInfo);

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
    return m_Swapchain->Present(m_FrameIndex++, m_PresentQueue.handle);
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

template<typename T, typename C>
void VulkanDevice::RemoveResource(T resource, C& container)
{
    auto ptr = Get(resource);
    auto it = std::find_if(container.begin(), container.end(), [&](auto& p)
    {
        return p.get() == ptr;
    });

    if (it != container.end())
        container.erase(it);
}

void VulkanDevice::WaitIdle()
{
    vkDeviceWaitIdle(m_Handle);
}

void VulkanDevice::RebuildSwapchain()
{
    m_Swapchain.reset();
    m_Swapchain = std::make_unique<VulkanSwapchain>(this);
}

}
