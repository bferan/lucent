#pragma once

#include "scene/Scene.hpp"

namespace lucent
{

struct RenderSettings
{
    uint32 viewportWidth = 1;
    uint32 viewportHeight = 1;

    uint32 framesInFlight = 3;
};

}
