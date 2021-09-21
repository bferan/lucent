#pragma once

#include "scene/Scene.hpp"

namespace lucent
{

struct RenderSettings
{
    uint32 viewportWidth;
    uint32 viewportHeight;

    Scene& scene;
};

}
