#pragma once

#include "device/Texture.hpp"

namespace lucent
{

constexpr int kMaxColorAttachments = 8;
constexpr int kMaxAttachments = kMaxColorAttachments + 1;

struct FramebufferSettings
{
    Array<Texture*, kMaxColorAttachments> colorTextures = {};
    int colorLayer = -1;
    int colorLevel = -1;

    Texture* depthTexture = nullptr;
    int depthLayer = -1;
    int depthLevel = -1;
};

//! Represents a collection of image attachments for rendering
class Framebuffer
{
public:
    const FramebufferSettings& GetSettings() const { return m_Settings; }

protected:
    FramebufferSettings m_Settings;
};

}
