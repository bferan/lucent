#pragma once

#include "core/Math.hpp"
#include "core/Matrix4.hpp"
#include "core/Vector4.hpp"
#include "device/Device.hpp"

// Descriptor management
// Uniform buffer MVP
// Textures

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
    void Draw();

private:
    void ImportModel();

public:
    Device m_Device;

    Buffer* m_VertexBuffer;
    Buffer* m_IndexBuffer;
    uint32_t m_numIndices;

    Pipeline* m_Pipeline;
    Context* m_Context;

    Buffer* m_UniformBuffer;
    DescriptorSet* m_DescSet;

};

}