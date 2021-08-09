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

    Glyph GetGlyph(char c) const;

    float PixelHeight() const
    {
        return m_PixelHeight;
    }

    void Bind(Context& context) const;

private:
    float m_PixelHeight;

    BakedFont* m_BakedFont;

    // Render data
    Device* m_Device;
    Pipeline* m_FontPipeline;
    Texture* m_FontTexture;
};

}
