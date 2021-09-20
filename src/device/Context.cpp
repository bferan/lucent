#include "device/Context.hpp"

namespace lucent
{

void Context::BindBuffer(DescriptorID id, const Buffer* buffer)
{
    BindBuffer(BoundPipeline()->shader->Lookup(id), buffer);
}

void Context::BindBuffer(DescriptorID id, const Buffer* buffer, uint32 dynamicOffset)
{
    BindBuffer(BoundPipeline()->shader->Lookup(id), buffer, dynamicOffset);
}

void Context::BindTexture(DescriptorID id, const Texture* texture, int level)
{
    BindTexture(BoundPipeline()->shader->Lookup(id), texture, level);
}

void Context::BindImage(DescriptorID id, const Texture* texture, int level)
{
    BindImage(BoundPipeline()->shader->Lookup(id), texture, level);
}

}