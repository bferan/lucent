#pragma once

#include "rendering/Renderer.hpp"
#include "rendering/RenderSettings.hpp"
#include "GeometryPass.hpp"

namespace lucent
{

//! Perform ground-truth ambient occlusion on given screen-space data; returns the AO texture
Texture* AddGTAOPass(Renderer& renderer, GBuffer gBuffer, Texture* hiZ);

}
