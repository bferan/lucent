#include "SceneRenderer.hpp"

#include "core/Utility.hpp"
#include "scene/ComponentPool.hpp"

namespace lucent
{

SceneRenderer::SceneRenderer(Device* device)
    : m_Device(device)
{
    m_DefaultPipeline = m_Device->CreatePipeline(PipelineInfo{
        .name = "Standard.shader",
        .source = ReadFile("C:/Code/lucent/src/rendering/shaders/Standard.shader")
    });

    m_SkyboxPipeline = m_Device->CreatePipeline(PipelineInfo{
        .name = "Skybox.shader",
        .source = ReadFile("C:/Code/lucent/src/rendering/shaders/Skybox.shader")
    });

    m_Context = m_Device->CreateContext();

    // Create UBO
    m_GlobalSet = m_Device->CreateDescriptorSet(*m_DefaultPipeline, 0);
    m_SkyboxSet = m_Device->CreateDescriptorSet(*m_SkyboxPipeline, 1);

    m_UniformBuffer = m_Device->CreateBuffer(BufferType::Uniform, 65535);
    m_Device->WriteSet(m_GlobalSet, 0, *m_UniformBuffer);

    // Create debug console (temp)
    m_DebugConsole = std::make_unique<DebugConsole>(m_Device);
}

void SceneRenderer::Render(Scene& scene)
{
    // Find first camera
    LC_ASSERT(scene.cameras.Size() > 0);
    auto cameraEntity = *scene.cameras.begin();

    auto& transform = scene.transforms[cameraEntity];
    auto& camera = scene.cameras[cameraEntity];

    // Calc camera position
    auto view = Matrix4::RotationX(-camera.pitch) *
        Matrix4::RotationY(-camera.yaw) *
        Matrix4::Translation(-transform.position);

    auto proj = Matrix4::Perspective(camera.horizontalFov, camera.aspectRatio, camera.near, camera.far);

    m_Device->WriteSet(m_GlobalSet, 1, *scene.environment.irradianceMap);
    m_Device->WriteSet(m_GlobalSet, 2, *scene.environment.specularMap);
    m_Device->WriteSet(m_GlobalSet, 3, *scene.environment.BRDF);

    m_Device->WriteSet(m_SkyboxSet, 0, *scene.environment.cubeMap);

    // Draw
    auto& ctx = *m_Context;
    auto& fb = m_Device->AcquireFramebuffer();

    ctx.Begin();
    ctx.BeginRenderPass(fb);

    ctx.Bind(*m_DefaultPipeline);

    const size_t uniformAlignment = m_Device->m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32_t uniformOffset = 0;

    // Draw entities
    for (auto entity : scene.meshInstances)
    {
        auto& local = scene.transforms[entity];
        auto& instance = scene.meshInstances[entity];

        auto model = Matrix4::Translation(local.position) *
            Matrix4::Rotation(local.rotation) *
            Matrix4::Scale(local.scale, local.scale, local.scale);

        UBO ubo = {
            .model = model,
            .view = view,
            .proj = proj,
            .cameraPos = transform.position
        };

        m_UniformBuffer->Upload(&ubo, sizeof(UBO), uniformOffset);

        for (auto idx : instance.meshes)
        {
            auto& mesh = scene.meshes[idx];
            auto& material = scene.materials[mesh.materialIdx];

            ctx.BindSet(m_GlobalSet, uniformOffset);
            ctx.BindSet(material.descSet);

            ctx.Bind(mesh.vertexBuffer, 0);
            ctx.Bind(mesh.indexBuffer);
            ctx.Draw(mesh.numIndices);
        }

        uniformOffset += sizeof(UBO);
        // Align up
        uniformOffset += (uniformAlignment - (uniformOffset % uniformAlignment)) % uniformAlignment;
    }

    // Draw skybox
    UBO ubo = {
        .model = Matrix4::Identity(),
        .view = view,
        .proj = proj
    };
    m_UniformBuffer->Upload(&ubo, sizeof(UBO), uniformOffset);
    ctx.Bind(*m_SkyboxPipeline);
    ctx.BindSet(m_GlobalSet, uniformOffset);
    ctx.BindSet(m_SkyboxSet);

    ctx.Bind(m_Device->m_Cube.indices);
    ctx.Bind(m_Device->m_Cube.vertices, 0);
    ctx.Draw(m_Device->m_Cube.numIndices);

    // Draw console
    m_DebugConsole->Render(ctx);

    ctx.EndRenderPass();
    ctx.End();

    m_Device->Submit(&ctx);
    m_Device->Present();
}

}
