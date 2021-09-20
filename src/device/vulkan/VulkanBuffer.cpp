#include "VulkanBuffer.hpp"

namespace lucent
{

// Buffer
void VulkanBuffer::Upload(const void* data, size_t size, size_t offset)
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

void VulkanBuffer::Clear(size_t size, size_t offset)
{
    LC_ASSERT(offset + size <= bufSize);

    uint8* ptr;
    vmaMapMemory(device->m_Allocator, allocation, (void**)&ptr);
    ptr += offset;
    memset(ptr, 0, size);
    vmaUnmapMemory(device->m_Allocator, allocation);
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

void* VulkanBuffer::Map()
{
    void* data;
    auto result = vmaMapMemory(device->m_Allocator, allocation, &data);
    LC_ASSERT(result == VK_SUCCESS);
    return data;
}

void VulkanBuffer::Flush(size_t size, size_t offset)
{
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

void VulkanBuffer::Invalidate(size_t size, size_t offset)
{
    vmaInvalidateAllocation(device->m_Allocator, allocation, offset, size);
}

}
