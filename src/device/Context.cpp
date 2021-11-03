#include "device/Context.hpp"

namespace lucent
{

void Context::BindBuffer(DescriptorID id, const Buffer* buffer)
{
    BindBuffer(BoundPipeline()->Lookup(id), buffer);
}

void Context::BindBuffer(DescriptorID id, const Buffer* buffer, uint32 dynamicOffset)
{
    BindBuffer(BoundPipeline()->Lookup(id), buffer, dynamicOffset);
}

void Context::BindTexture(DescriptorID id, const Texture* texture, int level)
{
    BindTexture(BoundPipeline()->Lookup(id), texture, level);
}

void Context::BindImage(DescriptorID id, const Texture* texture, int level)
{
    BindImage(BoundPipeline()->Lookup(id), texture, level);
}

}