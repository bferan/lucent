#pragma once

#include "rendering/Renderer.hpp"
#include "rendering/GeometryPass.hpp"

namespace lucent
{

Texture* AddScreenSpaceReflectionsPass(Renderer& renderer, GBuffer gBuffer, Texture* minZ, Texture* prevColor);

}
