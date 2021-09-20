#pragma once

namespace lucent
{

class Device;

enum class BufferType
{
    kVertex,
    kIndex,
    kUniform,
    kUniformDynamic,
    kStorage,
    kStaging
};

struct Buffer
{
    virtual void Upload(const void* data, size_t size, size_t offset = 0) = 0;

    virtual void Clear(size_t size, size_t offset) = 0;

    virtual void* Map() = 0;

    virtual void Flush(size_t size, size_t offset) = 0;

    virtual void Invalidate(size_t size, size_t offset) = 0;
};

}
