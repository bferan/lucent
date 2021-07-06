#include "SceneRenderer.hpp"

#include "core/Utility.hpp"
#include "scene/ComponentPool.hpp"

namespace lucent
{

SceneRenderer::SceneRenderer(Device* device)
    : m_Device(device)
{
    m_Pipeline = m_Device->CreatePipeline(PipelineInfo{
        .programInfo = {
            .vertShader = ReadFile("C:/Code/lucent/src/rendering/vert.glsl"),
            .fragShader = ReadFile("C:/Code/lucent/src/rendering/frag.glsl")
        }
    });

    m_Context = m_Device->CreateContext();

    // Create UBO
    m_GlobalSet = m_Device->CreateDescriptorSet(*m_Pipeline, 0);
    m_UniformBuffer = m_Device->CreateBuffer(BufferType::Uniform, 65535);
    m_Device->WriteSet(m_GlobalSet, 0, *m_UniformBuffer);
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

    // Draw
    auto& ctx = *m_Context;
    auto& fb = m_Device->AcquireFramebuffer();

    ctx.Begin();
    ctx.BeginRenderPass(fb);

    ctx.Bind(*m_Pipeline);

    const size_t uniformAlignment = m_Device->m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32_t uniformOffset = 0;

    for (auto entity : scene.meshInstances)
    {
        auto& mesh = scene.meshes[scene.meshInstances[entity].meshIndex];
        auto& local = scene.transforms[entity];

        auto model = Matrix4::Translation(local.position) *
            Matrix4::Rotation(local.rotation) *
            Matrix4::Scale(local.scale, local.scale, local.scale);

        UBO ubo = {
            .model = model,
            .view = view,
            .proj = proj,
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

    m_Device->Submit(&ctx);
    m_Device->Present();
}

}
