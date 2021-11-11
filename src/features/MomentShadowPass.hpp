#pragma once

#include "rendering/Renderer.hpp"
#include "rendering/RenderSettings.hpp"

namespace lucent
{

//! Returns the moment shadow map array for the main directional light
Texture* AddMomentShadowPass(Renderer& renderer);

}
