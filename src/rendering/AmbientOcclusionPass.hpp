#pragma once

#include "rendering/Renderer.hpp"
#include "rendering/RenderSettings.hpp"
#include "rendering/GeometryPass.hpp"

namespace lucent
{

Texture* AddGTAOPass(Renderer& renderer, GBuffer gBuffer, Texture* hiZ);

}
