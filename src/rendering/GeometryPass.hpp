#pragma once

#include "device/Device.hpp"
#include "rendering/Renderer.hpp"
#include "rendering/RenderSettings.hpp"
#include "scene/Scene.hpp"

namespace lucent
{

struct GBuffer
{
    Texture* baseColor;
    Texture* depth;
    Texture* normals;
    Texture* metalRoughness;
};

GBuffer AddGeometryPass(Renderer& renderer, const RenderSettings& settings);

Texture* AddGenerateHiZPass(Renderer& renderer, const RenderSettings& settings, Texture* depthTexture);

}
