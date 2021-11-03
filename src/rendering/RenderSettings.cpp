#include "RenderSettings.hpp"

namespace lucent
{

std::pair<uint32, uint32> RenderSettings::GetNumGroups(uint32 width, uint32 height) const
{
    auto x = (uint32)Ceil((float)width / (float)defaultGroupSizeX);
    auto y = (uint32)Ceil((float)height / (float)defaultGroupSizeY);
    return { x, y };
}

}
