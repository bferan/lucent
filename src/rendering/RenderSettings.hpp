#pragma once

#include "scene/Scene.hpp"

namespace lucent
{

struct RenderSettings
{
    uint32 viewportWidth = 1;
    uint32 viewportHeight = 1;

    uint32 framesInFlight = 1;

    uint32 defaultGroupSizeX = 8;
    uint32 defaultGroupSizeY = 8;
    std::pair<uint32, uint32> GetNumGroups(uint32 width, uint32 height) const;

    Texture* defaultBlackTexture;
    Texture* defaultWhiteTexture;
    Texture* defaultGrayTexture;
    Texture* defaultGreenTexture;
    Texture* defaultNormalTexture;

    // Cube mesh
    // Quad mesh
    // Sphere mesh

    // BRDF LUT texture
    // Blue noise texture
};

}
