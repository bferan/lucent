#include "VulkanBuffer.hpp"

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

VulkanBuffer::VulkanBuffer(VulkanDevice* dev, BufferType bufType, size_t bufSize)
    : device(dev)
    , handle()
    , allocation()
    , type(bufType)
    , capacity(bufSize)
{
    auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };

    auto bufferInfo = VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = capacity,
        .usage = BufferTypeToFlags(type),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    LC_CHECK(vmaCreateBuffer(device->GetAllocator(), &bufferInfo, &allocInfo, &handle, &allocation, nullptr));
}

VulkanBuffer::~VulkanBuffer()
{
    vmaDestroyBuffer(device->GetAllocator(), handle, allocation);
}

void VulkanBuffer::Upload(const void* data, size_t size, size_t offset)
{
    LC_ASSERT(offset + size <= capacity);

    uint8* ptr;
    vmaMapMemory(device->m_Allocator, allocation, (void**)&ptr);
    ptr += offset;
    memcpy(ptr, data, size);
    vmaUnmapMemory(device->m_Allocator, allocation);
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

void VulkanBuffer::Clear(size_t size, size_t offset)
{
    LC_ASSERT(offset + size <= capacity);

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
    LC_CHECK(vmaMapMemory(device->m_Allocator, allocation, &data));
    return data;
}

void VulkanBuffer::Unmap()
{
    vmaUnmapMemory(device->m_Allocator, allocation);
}

void VulkanBuffer::Flush(size_t size, size_t offset)
{
    vmaFlushAllocation(device->m_Allocator, allocation, offset, size);
}

void VulkanBuffer::Invalidate(size_t size, size_t offset)
{
    vmaInvalidateAllocation(device->m_Allocator, allocation, offset, size);
}

BufferType VulkanBuffer::GetType()
{
    return type;
}

}
