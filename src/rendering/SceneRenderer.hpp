#pragma once

#include "device/Device.hpp"

namespace lucent
{

class SceneRenderer
{
public:
    SceneRenderer(Device* device);

private:
    Device* m_Device;

};

}
