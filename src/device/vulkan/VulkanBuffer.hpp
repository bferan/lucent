#pragma once

#include "VulkanCommon.hpp"
#include "device/Buffer.hpp"
#include "device/vulkan/VulkanDevice.hpp"

namespace lucent
{

class VulkanBuffer : public Buffer
{
public:
    explicit VulkanBuffer(VulkanDevice* device, BufferType type, size_t size);
    ~VulkanBuffer();

    void Upload(const void* data, size_t size, size_t offset) override;
    void Clear(size_t size, size_t offset) override;
    void* Map() override;
    void Unmap() override;
    BufferType GetType() override;

public:
    VulkanDevice* device;
    VkBuffer handle;
    VmaAllocation allocation;
    BufferType type;
    size_t capacity;
    void* mappedPointer;
};

}
