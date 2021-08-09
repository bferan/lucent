#pragma once

#include "device/Device.hpp"
#include "scene/Scene.hpp"
#include "debug/DebugConsole.hpp"

namespace lucent
{

struct UBO
{
    Matrix4 model;
    Matrix4 view;
    Matrix4 proj;

    Vector3 cameraPos;
};

class SceneRenderer
{
public:
    explicit SceneRenderer(Device* device);

    void Render(Scene& scene);

public:
    Device* m_Device;
    Context* m_Context;

    Pipeline* m_DefaultPipeline;
    Pipeline* m_SkyboxPipeline;

    DescriptorSet* m_SkyboxSet;

    // UBO
    DescriptorSet* m_GlobalSet;
    Buffer* m_UniformBuffer;

    // TEMP
    std::unique_ptr<DebugConsole> m_DebugConsole;
};

}
