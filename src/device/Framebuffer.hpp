#pragma once

#include "device/Texture.hpp"

namespace lucent
{

constexpr int kMaxColorAttachments = 8;
constexpr int kMaxAttachments = kMaxColorAttachments + 1;

enum class FramebufferUsage
{
    kSwapchainImage,
    kDefault
};

struct FramebufferSettings
{
    FramebufferUsage usage = FramebufferUsage::kDefault;

    Array<Texture*, kMaxColorAttachments> colorTextures = {};
    int colorLayer = -1;
    int colorLevel = -1;

    Texture* depthTexture = nullptr;
    int depthLayer = -1;
    int depthLevel = -1;
};

struct Framebuffer
{
    FramebufferUsage usage{};

    Array<Texture*, kMaxColorAttachments> colorTextures;
    Texture* depthTexture{};
};

}
