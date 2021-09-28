#pragma once

#include "debug/DebugConsole.hpp"
#include "rendering/Renderer.hpp"

namespace lucent
{

void AddDebugOverlayPass(Renderer& renderer, DebugConsole* console, Texture* output);

}
