#pragma once

#include "rendering/Renderer.hpp"
#include "GeometryPass.hpp"

namespace lucent
{

Texture* CreateSceneRadianceTarget(Renderer& renderer);

void AddLightingPass(Renderer& renderer,
    GBuffer gBuffer,
    Texture* depth,
    Texture* sceneRadiance,
    Texture* momentShadows,
    Texture* screenAO,
    Texture* screenReflections);

}
