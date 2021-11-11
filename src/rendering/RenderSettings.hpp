#pragma once

#include "scene/Scene.hpp"

namespace lucent
{

struct RenderSettings
{
public:
    std::pair<uint32, uint32> ComputeGroupCount(uint32 width, uint32 height) const;

    void InitializeDefaultResources(Device* device);

public:
    uint32 viewportWidth = 1600;
    uint32 viewportHeight = 900;
    const char* viewportName = "Lucent";

    uint32 framesInFlight = 3;

    uint32 defaultGroupSizeX = 8;
    uint32 defaultGroupSizeY = 8;

    Texture* defaultBlackTexture;
    Texture* defaultWhiteTexture;
    Texture* defaultGrayTexture;
    Texture* defaultGreenTexture;
    Texture* defaultNormalTexture;

    using DefaultMesh = std::unique_ptr<StaticMesh>;
    DefaultMesh cubeMesh;
    DefaultMesh quadMesh;
    DefaultMesh sphereMesh;
};

}
