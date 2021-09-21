#pragma once

#include "rendering/Renderer.hpp"

namespace lucent
{

Texture* AddMomentShadowPass(Renderer& renderer, const RenderSettings& settings)
{
    uint32 width;
    uint32 height;

    auto moments = renderer.AddRenderTarget(TextureSettings{
        .width = DirectionalLight::kMapWidth,
        .height = DirectionalLight::kMapWidth,
        .layers = DirectionalLight::kNumCascades,
        .format = TextureFormat::kRGBA32F,
        .shape = TextureShape::k2DArray,
        .addressMode = TextureAddressMode::kClampToBorder
    });


}


}
