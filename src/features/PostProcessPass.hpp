#pragma once

#include "rendering/Renderer.hpp"

namespace lucent
{

Texture* AddPostProcessPass(Renderer& renderer, Texture* sceneRadiance);

}
