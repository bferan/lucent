#pragma once

#include "device/Device.hpp"

namespace lucent
{

struct BakedFont;

struct Glyph
{
    // Bottom left
    Vector2 minPos;
    Vector2 minTexCoord;

    // Top right
    Vector2 maxPos;
    Vector2 maxTexCoord;

    float advance;
};

class Font
{
public:
    Font(Device* device, const std::string& fontFile, float pixelHeight);

    ~Font();

    Texture* GetAtlas() const;

    Glyph GetGlyph(char c) const;

    float GetPixelHeight() const
    {
        return m_PixelHeight;
    }
private:
    float m_PixelHeight;
    BakedFont* m_BakedFont;
    Texture* m_FontTexture;
};

}
