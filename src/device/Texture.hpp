#pragma once

namespace lucent
{

enum class TextureFormat
{
    kR8,
    kRGBA8_SRGB,
    kRGBA8,
    kRGBA32F,
    kRG32F,
    kR32F,

    kDepth16U,
    kDepth32F
};

enum class TextureShape
{
    k2D,
    k2DArray,
    kCube
};

enum class TextureAddressMode
{
    kRepeat,
    kClampToEdge,
    kClampToBorder
};

enum class TextureFilter
{
    kLinear,
    kNearest
};

enum class TextureUsage
{
    kReadOnly,
    kReadWrite,
    kPresentSrc,
    kDepthAttachment
};

constexpr uint32 kAllLayers = ~0u;

struct TextureSettings
{
    uint32 width = 1;
    uint32 height = 1;
    uint32 levels = 1;
    uint32 layers = 1;
    uint32 samples = 1;
    TextureFormat format = TextureFormat::kRGBA8;
    TextureShape shape = TextureShape::k2D;
    TextureAddressMode addressMode = TextureAddressMode::kRepeat;
    TextureFilter filter = TextureFilter::kLinear;
    TextureUsage usage = TextureUsage::kReadOnly;
};

struct Texture
{
    virtual void Upload(size_t size, const void* data) = 0;

    TextureUsage usage;
    uint32 width;
    uint32 height;
    uint32 levels;
};

}
