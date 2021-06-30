#pragma once

#include "core/Math.hpp"
#include "core/Matrix4.hpp"
#include "core/Vector4.hpp"
#include "device/Device.hpp"
#include "scene/Scene.hpp"

namespace lucent::demos
{

struct UBO
{
    Matrix4 model;
    Matrix4 view;
    Matrix4 proj;
    Vector3 col;
};

class BasicDemo
{
public:
    void Init();
    void Draw(float dt);

public:
    Device m_Device;
    Pipeline* m_Pipeline;
    Context* m_Context;

    // UBO
    DescriptorSet* m_GlobalSet;
    Buffer* m_UniformBuffer;

    Scene m_Scene{};
};

}