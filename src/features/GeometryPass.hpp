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
    Texture* emissive;
};

GBuffer AddGeometryPass(Renderer& renderer);

Texture* AddGenerateHiZPass(Renderer& renderer, Texture* depthTexture);

}
