#include "Basic.hpp"

#include <iostream>
#include <fstream>

#include "GLFW/glfw3.h"

#include "core/Utility.hpp"
#include "device/Device.hpp"
#include "core/Vector3.hpp"
#include "scene/Scene.hpp"
#include "scene/Importer.hpp"

namespace lucent::demos
{

void BasicDemo::Init()
{
    m_Pipeline = m_Device.CreatePipeline(PipelineInfo{
        .vertexShader = ReadFile("C:/Code/lucent/src/rendering/vert.glsl"),
        .fragmentShader = ReadFile("C:/Code/lucent/src/rendering/frag.glsl")
    });

    Importer importer(&m_Device, m_Pipeline);

    auto helm = importer.Import(m_Scene, "models/DamagedHelmet.glb");
    m_Scene.transforms[helm].position.x = -1.0f;

    auto avo = importer.Import(m_Scene, "models/Avocado.glb");
    m_Scene.transforms[avo].scale = 40.0f;

    m_Context = m_Device.CreateContext();
    m_GlobalSet = m_Device.CreateDescriptorSet(*m_Pipeline, 0);

    // Create UBO
    m_UniformBuffer = m_Device.CreateBuffer(BufferType::Uniform, 65535);
    m_Device.WriteSet(m_GlobalSet, 0, *m_UniformBuffer);
}

void BasicDemo::Draw(float dt)
{
    static float timer = 0.0f;
    timer += dt;

    // Draw
    auto& ctx = *m_Context;
    auto& fb = m_Device.AcquireFramebuffer();

    ctx.Begin();
    ctx.BeginRenderPass(fb);

    ctx.Bind(*m_Pipeline);

    const size_t uniformAlignment = m_Device.m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32_t uniformOffset = 0;

    for (auto entity : m_Scene.meshInstances)
    {
        auto& mesh = m_Scene.meshes[m_Scene.meshInstances[entity].meshIndex];
        auto& local = m_Scene.transforms[entity];

        auto model = Matrix4::Translation(local.position) *
            Matrix4::Rotation(local.rotation) *
            Matrix4::Scale(local.scale, local.scale, local.scale);

        UBO ubo = {
            .model = model * Matrix4::RotationY(kPi * timer * 0.1f),
            .view = Matrix4::Translation(Vector3{ 0.0f, 0.0f, -3.0f }),
            .proj = Matrix4::Perspective(1, 1600.0f / 900.0f, 0.01, 10000),
            .col = Vector3{ 0.0f, 0.0f, 1.0f }
        };

        m_UniformBuffer->Upload(&ubo, sizeof(UBO), uniformOffset);

        ctx.BindSet(m_GlobalSet, uniformOffset);
        ctx.BindSet(mesh.descSet);

        ctx.Bind(mesh.vertexBuffer, 0);
        ctx.Bind(mesh.indexBuffer);

        ctx.Draw(mesh.numIndices);

        uniformOffset += sizeof(UBO);
        // Align up
        uniformOffset += (uniformAlignment - (uniformOffset % uniformAlignment)) % uniformAlignment;
    }

    ctx.EndRenderPass();
    ctx.End();

    m_Device.Submit(&ctx);
    m_Device.Present();
}

}

int main()
{
    lucent::demos::BasicDemo demo;
    demo.Init();

    double prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(demo.m_Device.m_Window))
    {
        glfwPollEvents();
        double time = glfwGetTime();
        demo.Draw(static_cast<float>(time - prevTime));
        prevTime = time;
    }
    return 0;
}
