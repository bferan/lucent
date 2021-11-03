#pragma once

namespace lucent
{

enum class TextureFormat
{
    // Unsigned normalized
    kR8,
    kRG8,
    kRGB8,
    kRGBA8,
    kRGBA8_sRGB,

    kRGB10A2,

    // Floating point
    kR32F,
    kRG32F,
    kRGB32F,
    kRGBA32F,

    // Depth
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
    bool generateMips = false;
};

class Texture
{
public:
    const TextureSettings& GetSettings() const { return m_Settings; }

    virtual void Upload(size_t size, const void* data) = 0;

protected:
    TextureSettings m_Settings;
};

}
