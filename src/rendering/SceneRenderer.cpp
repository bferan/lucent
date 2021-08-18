#include "SceneRenderer.hpp"

#include "core/Utility.hpp"
#include "device/Context.hpp"

namespace lucent
{

const int kShadowMapRes = 1024;

SceneRenderer::SceneRenderer(Device* device)
    : m_Device(device)
{
    m_DefaultPipeline = m_Device->CreatePipeline(PipelineSettings{ .shaderName = "Standard.shader" });
    m_SkyboxPipeline = m_Device->CreatePipeline(PipelineSettings{ .shaderName = "Skybox.shader" });

    m_Context = m_Device->CreateContext();

    // Create debug console (temp)
    m_DebugConsole = std::make_unique<DebugConsole>(m_Device);

    // Shadows
    m_ShadowMap = m_Device->CreateTexture(TextureSettings{
        .width = kShadowMapRes,
        .height = kShadowMapRes,
        .format = TextureFormat::kR32F,
        .addressMode = TextureAddressMode::kClampToBorder
    });

    m_ShadowDepth = m_Device->CreateTexture(TextureSettings{
        .width = kShadowMapRes,
        .height = kShadowMapRes,
        .format = TextureFormat::kDepth
    });

    m_ShadowFramebuffer = m_Device->CreateFramebuffer(FramebufferSettings{
        .colorTexture = m_ShadowMap,
        .depthTexture = m_ShadowDepth
    });

    m_ShadowPipeline = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "StandardShadow.shader",
        .framebuffer = m_ShadowFramebuffer
    });
}

void SceneRenderer::Render(Scene& scene)
{
    // Find first camera
    auto& transform = scene.mainCamera.Get<Transform>();
    auto& camera = scene.mainCamera.Get<Camera>();

    // Calc camera position
    auto view = Matrix4::RotationX(kPi) *
        Matrix4::RotationX(-camera.pitch) *
        Matrix4::RotationY(-camera.yaw) *
        Matrix4::Translation(-transform.position);

    auto proj = Matrix4::Perspective(camera.horizontalFov, camera.aspectRatio, camera.near, camera.far);

    // Draw
    auto& ctx = *m_Context;
    ctx.Begin();

    RenderShadows(scene);

    ctx.BeginRenderPass(m_Device->AcquireFramebuffer());

    ctx.Bind(m_DefaultPipeline);

    // Draw entities
    scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
    {
        for (auto idx : instance.meshes)
        {
            auto& mesh = scene.meshes[idx];
            auto& material = scene.materials[mesh.materialIdx];

            // Bind globals
            ctx.Uniform("u_Model"_id, local.model);
            ctx.Uniform("u_View"_id, view);
            ctx.Uniform("u_Proj"_id, proj);
            ctx.Uniform("u_CameraPos"_id, transform.position);
            ctx.Uniform("u_LightViewProj"_id, m_ShadowViewProj);
            ctx.Uniform("u_LightDir"_id, m_ShadowDir);

            ctx.Bind("u_EnvIrradiance"_id, scene.environment.irradianceMap);
            ctx.Bind("u_EnvSpecular"_id, scene.environment.specularMap);
            ctx.Bind("u_BRDF"_id, scene.environment.BRDF);
            ctx.Bind("u_ShadowMap"_id, m_ShadowMap);

            ctx.Bind("u_BaseColor"_id, material.baseColorMap);
            ctx.Bind("u_MetalRoughness"_id, material.metalRough);
            ctx.Bind("u_Normal"_id, material.normalMap);
            ctx.Bind("u_AO"_id, material.aoMap);
            ctx.Bind("u_Emissive"_id, material.emissive);

            ctx.Bind(mesh.vertexBuffer);
            ctx.Bind(mesh.indexBuffer);
            ctx.Draw(mesh.numIndices);
        }
    });

    // Draw skybox
    ctx.Bind(m_SkyboxPipeline);

    ctx.Uniform("u_Model"_id, Matrix4::Identity());
    ctx.Uniform("u_View"_id, view);
    ctx.Uniform("u_Proj"_id, proj);

    ctx.Bind("u_EnvIrradiance"_id, scene.environment.irradianceMap);
    ctx.Bind("u_EnvSpecular"_id, scene.environment.specularMap);
    ctx.Bind("u_BRDF"_id, scene.environment.BRDF);
    ctx.Bind("u_ShadowMap"_id, m_ShadowMap);

    ctx.Bind("u_Environment"_id, scene.environment.cubeMap);

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

void SceneRenderer::RenderShadows(Scene& scene)
{
    auto& ctx = *m_Context;

    // Find view & projection matrix of directional light
    auto& light = scene.mainDirectionalLight.Get<Transform>();

    m_ShadowViewProj =
        Matrix4::Orthographic(10.0f, 10.0f, 15.0f) *
        Matrix4::RotationX(kPi) *
        Matrix4::Rotation(light.rotation.Inverse()) *
        Matrix4::Translation(-light.position);

    m_ShadowDir = -Vector3(Matrix4(light.rotation).c3);

    ctx.TransitionImage(m_ShadowMap);
    ctx.BeginRenderPass(m_ShadowFramebuffer);

    ctx.Bind(m_ShadowPipeline);

    scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
    {
        for (auto idx : instance.meshes)
        {
            auto& mesh = scene.meshes[idx];
            auto mvp = m_ShadowViewProj * local.model;

            // Bind globals
            ctx.Uniform("u_MVP"_id, mvp);

            ctx.Bind(mesh.vertexBuffer);
            ctx.Bind(mesh.indexBuffer);
            ctx.Draw(mesh.numIndices);
        }
    });

    ctx.EndRenderPass();
    ctx.TransitionAttachment(m_ShadowMap);
}

}
