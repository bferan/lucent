#pragma once

#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

#define LC_CHECK(expr) if ((expr) != VK_SUCCESS) { LC_ERROR("ERROR: Vulkan returned non-success: " #expr); }

namespace lucent
{

// Forward declarations
class Pipeline;
class Framebuffer;
class Texture;
class Buffer;
class Context;
class ShaderCache;

class VulkanDevice;
class VulkanPipeline;
class VulkanFramebuffer;
class VulkanTexture;
class VulkanBuffer;
class VulkanContext;
class VulkanSwapchain;

// Convenience downcasting
static VulkanPipeline* Get(Pipeline* pipeline) { return reinterpret_cast<VulkanPipeline*>(pipeline); }
static VulkanFramebuffer* Get(Framebuffer* framebuffer) { return reinterpret_cast<VulkanFramebuffer*>(framebuffer); }
static VulkanTexture* Get(Texture* texture) { return reinterpret_cast<VulkanTexture*>(texture); }
static VulkanBuffer* Get(Buffer* buffer) { return reinterpret_cast<VulkanBuffer*>(buffer); }
static VulkanContext* Get(Context* context) { return reinterpret_cast<VulkanContext*>(context); }

static const VulkanPipeline* Get(const Pipeline* pipeline) { return reinterpret_cast<const VulkanPipeline*>(pipeline); }
static const VulkanFramebuffer* Get(const Framebuffer* framebuffer) { return reinterpret_cast<const VulkanFramebuffer*>(framebuffer); }
static const VulkanTexture* Get(const Texture* texture) { return reinterpret_cast<const VulkanTexture*>(texture); }
static const VulkanBuffer* Get(const Buffer* buffer) { return reinterpret_cast<const VulkanBuffer*>(buffer); }
static const VulkanContext* Get(const Context* context) { return reinterpret_cast<const VulkanContext*>(context); }

}

