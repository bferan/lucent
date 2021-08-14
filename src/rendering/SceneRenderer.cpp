#include "SceneRenderer.hpp"

#include "core/Utility.hpp"
#include "device/Context.hpp"

namespace lucent
{

SceneRenderer::SceneRenderer(Device* device)
    : m_Device(device)
{
    m_DefaultPipeline = m_Device->CreatePipeline(PipelineSettings{ .shaderName = "Standard.shader" });
    m_SkyboxPipeline = m_Device->CreatePipeline(PipelineSettings{ .shaderName = "Skybox.shader" });

    m_Context = m_Device->CreateContext();

    // Create UBO
    m_UniformBuffer = m_Device->CreateBuffer(BufferType::UniformDynamic, 65535);

    // Create debug console (temp)
    m_DebugConsole = std::make_unique<DebugConsole>(m_Device);
}

void SceneRenderer::Render(Scene& scene)
{
    // Find first camera
    auto& transform = scene.mainCamera.Get<Transform>();
    auto& camera = scene.mainCamera.Get<Camera>();

    // Calc camera position
    auto view = Matrix4::RotationX(-camera.pitch) *
        Matrix4::RotationY(-camera.yaw) *
        Matrix4::Translation(-transform.position);

    auto proj = Matrix4::Perspective(camera.horizontalFov, camera.aspectRatio, camera.near, camera.far);

    // Draw
    auto& ctx = *m_Context;
    auto& fb = m_Device->AcquireFramebuffer();

    ctx.Begin();
    ctx.BeginRenderPass(fb);

    ctx.Bind(m_DefaultPipeline);

    const size_t uniformAlignment = m_Device->m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32 uniformOffset = 0;

    // Draw entities
    scene.Each<MeshInstance, Transform>([&](auto& instance, auto& local)
    {
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

            // Bind globals
            ctx.Bind(0, 0, m_UniformBuffer, uniformOffset);
            ctx.Bind(0, 1, scene.environment.irradianceMap);
            ctx.Bind(0, 2, scene.environment.specularMap);
            ctx.Bind(0, 3, scene.environment.BRDF);

            ctx.Bind("u_BaseColor"_id, material.baseColorMap);
            ctx.Bind("u_MetalRoughness"_id, material.metalRough);
            ctx.Bind("u_Normal"_id, material.normalMap);
            ctx.Bind("u_AO"_id, material.aoMap);
            ctx.Bind("u_Emissive"_id, material.emissive);

            ctx.Bind(mesh.vertexBuffer);
            ctx.Bind(mesh.indexBuffer);
            ctx.Draw(mesh.numIndices);
        }

        uniformOffset += sizeof(UBO);
        // Align up
        uniformOffset += (uniformAlignment - (uniformOffset % uniformAlignment)) % uniformAlignment;
    });

    // Draw skybox
    UBO ubo = {
        .model = Matrix4::Identity(),
        .view = view,
        .proj = proj
    };
    m_UniformBuffer->Upload(&ubo, sizeof(UBO), uniformOffset);
    ctx.Bind(m_SkyboxPipeline);
    ctx.Bind(0, 0, m_UniformBuffer, uniformOffset);
    ctx.Bind(0, 1, scene.environment.irradianceMap);
    ctx.Bind(0, 2, scene.environment.specularMap);
    ctx.Bind(0, 3, scene.environment.BRDF);
    ctx.Bind(1, 0, scene.environment.cubeMap);

    ctx.Bind(m_Device->m_Cube.indices);
    ctx.Bind(m_Device->m_Cube.vertices);
    ctx.Draw(m_Device->m_Cube.numIndices);

    // Draw console
    m_DebugConsole->Render(ctx);

    ctx.EndRenderPass();
    ctx.End();

    m_Device->Submit(&ctx);
    m_Device->Present();
}

}
