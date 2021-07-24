#pragma once

#include <array>

#include "device/Device.hpp"
#include "debug/TextMesh.hpp"

namespace lucent
{


class DebugConsole
{
public:
    explicit DebugConsole(Device* device);

    ~DebugConsole();

    void Render(Context& ctx);

private:
    Device* m_Device;

    Font m_Font;
    TextMesh m_TextMesh;
};

}
