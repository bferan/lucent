#pragma once

#include "rendering/Renderer.hpp"

namespace lucent
{

//! Returns the output color texture with applied effects
Texture* AddPostProcessPass(Renderer& renderer, Texture* sceneRadiance);

}
