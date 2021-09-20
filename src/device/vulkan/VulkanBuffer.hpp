#pragma once

#include "VulkanCommon.hpp"
#include "device/Buffer.hpp"
#include "device/vulkan/VulkanDevice.hpp"

namespace lucent
{

class VulkanBuffer : public Buffer
{
public:
    void Upload(const void* data, size_t size, size_t offset) override;
    void Clear(size_t size, size_t offset) override;
    void* Map() override;
    void Flush(size_t size, size_t offset) override;
    void Invalidate(size_t size, size_t offset) override;

public:
    VulkanDevice* device{};
    VkBuffer handle{};
    VmaAllocation allocation{};
    BufferType type{};
    size_t bufSize{};
};

}
