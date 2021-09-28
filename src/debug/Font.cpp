#include "Font.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "core/Utility.hpp"

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
    : m_PixelHeight(pixelHeight)
{
    // Read TTF font file
    auto fontText = ReadFile(fontFile, std::ios::binary | std::ios::in);
    auto data = reinterpret_cast<const unsigned char*>(fontText.data());

    std::vector<uint8> bitmap;
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
    m_FontTexture = device->CreateTexture(texInfo);
    m_FontTexture->Upload(bitmap.size(), bitmap.data());
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

Font::~Font()
{
    delete m_BakedFont;
}

Texture* Font::GetAtlas() const
{
    return m_FontTexture;
}

}
