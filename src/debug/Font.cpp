#include "Font.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "core/Utility.hpp"
#include "core/Lucent.hpp"

namespace lucent
{

constexpr int kBitmapWidth = 512;
constexpr int kBitmapHeight = 512;
constexpr int kFirstAsciiChar = 32;
constexpr int kNumChars = 96;

struct BakedFont
{
    stbtt_bakedchar chars[kNumChars];
};

Font::Font(Device* device, const std::string& fontFile, float pixelHeight)
    : m_Device(device), m_PixelHeight(pixelHeight)
{
    // Read TTF font file
    auto fontText = ReadFile(fontFile, std::ios::binary | std::ios::in);
    auto data = reinterpret_cast<const unsigned char*>(fontText.data());

    std::vector<uint8_t> bitmap;
    bitmap.resize(kBitmapWidth * kBitmapHeight);

    m_BakedFont = new BakedFont();

    // Bake font characters
    stbtt_BakeFontBitmap(data, 0, pixelHeight, bitmap.data(), kBitmapWidth, kBitmapHeight,
        kFirstAsciiChar, kNumChars, m_BakedFont->chars);

    // Create and upload font texture
    auto texInfo = TextureSettings{
        .width = kBitmapWidth,
        .height = kBitmapHeight,
        .format = TextureFormat::kR8
    };
    m_FontTexture = m_Device->CreateTexture(texInfo, bitmap.size(), bitmap.data());

    // Create font pipeline & descriptor set
    m_FontPipeline = m_Device->CreatePipeline(PipelineSettings{
            .shaderName = "DebugFont.shader",
            .depthTestEnable = false,
        }
    );

    m_DescSet = m_Device->CreateDescriptorSet(*m_FontPipeline, 0);
    m_Device->WriteSet(m_DescSet, 0, *m_FontTexture);
}

Glyph Font::GetGlyph(char c) const
{
    auto charIndex = c - kFirstAsciiChar;

    LC_ASSERT(charIndex >= 0 && charIndex < kNumChars);

    const auto& baked = m_BakedFont->chars[charIndex];

    const float inverseW = 1.0f / kBitmapWidth;
    const float inverseH = 1.0f / kBitmapHeight;

    auto charW = (float)(baked.x1 - baked.x0);
    auto charH = (float)(baked.y1 - baked.y0);

    // Convert from stbtt's upper left origin to lower left
    // baked.yoff is offset of the glyph's upper-left corner (in stb space)
    return Glyph{
        .minPos = { baked.xoff, -baked.yoff - charH },
        .minTexCoord = { (float)baked.x0 * inverseW, (float)baked.y1 * inverseH },
        .maxPos = { charW + baked.xoff, -baked.yoff },
        .maxTexCoord = { (float)baked.x1 * inverseW, (float)baked.y0 * inverseH },
        .advance = baked.xadvance
    };
}

void Font::Bind(Context& context) const
{
    context.Bind(*m_FontPipeline);
    context.BindSet(m_DescSet);
}

Font::~Font()
{
    delete m_BakedFont;
}

}
