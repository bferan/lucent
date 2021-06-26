#pragma once

#include "core/Math.hpp"
#include "device/Device.hpp"

namespace lucent::demos
{

class BasicDemo
{
public:
    void Init();
    void Draw();

public:
    Device m_Device;

    Buffer* m_VertexBuffer;
    Buffer* m_IndexBuffer;

    Pipeline* m_Pipeline;
    Context* m_Context;
};

}