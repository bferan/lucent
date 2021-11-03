#pragma once

#include "rendering/Renderer.hpp"
#include "features/GeometryPass.hpp"

namespace lucent
{

Texture* AddScreenSpaceReflectionsPass(Renderer& renderer, GBuffer gBuffer, Texture* minZ, Texture* prevColor);

}
