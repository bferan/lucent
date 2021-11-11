#include "LightingPass.hpp"

#include "scene/Transform.hpp"

namespace lucent
{

struct alignas(sizeof(Vector4)) DirectionalLightParams
{
    Color color;
    Vector4 direction;
    Matrix4 proj;
    Vector4 plane[3];
    Vector4 scale[3];
    Vector4 offset[3];
};

Texture* CreateSceneRadianceTarget(Renderer& renderer)
{
    auto& settings = renderer.GetSettings();

    return renderer.AddRenderTarget(TextureSettings{
        .width = settings.viewportWidth,
        .height = settings.viewportHeight,
        .format = TextureFormat::kRGBA32F
    });
}

void AddLightingPass(Renderer& renderer,
    GBuffer gBuffer,
    Texture* depth,
    Texture* sceneRadiance,
    Texture* momentShadows,
    Texture* screenAO,
    Texture* screenReflections)
{
    auto framebuffer = renderer.AddFramebuffer(FramebufferSettings{
        .colorTextures = { sceneRadiance },
        .depthTexture = gBuffer.depth
    });

    auto lightingPipeline = renderer.AddPipeline(PipelineSettings{
        .shaderName = "LightingPass.shader",
        .framebuffer = framebuffer,
        .depthTestEnable = false,
        .depthWriteEnable = false
    });

    auto skyboxPipeline = renderer.AddPipeline(PipelineSettings{
        .shaderName = "Skybox.shader",
        .framebuffer = framebuffer,
        .depthWriteEnable = false
    });

    // Shared geometry primitives
    auto& settings = renderer.GetSettings();
    auto quad = settings.quadMesh.get();
    auto cube = settings.cubeMesh.get();

    renderer.AddPass("Lighting", [=](Context& ctx, View& view)
    {
        ctx.BeginRenderPass(framebuffer);

        ctx.BindPipeline(lightingPipeline);
        view.BindUniforms(ctx);

        // Bind directional light parameters
        auto light = view.GetScene().mainDirectionalLight;
        auto& dirLight = light.Get<DirectionalLight>();

        DirectionalLightParams params{};
        params.color = dirLight.color;
        params.direction =
            view.GetViewMatrix() * Vector4(light.Get<Transform>().TransformDirection(Vector3::Forward()), 0.0);
        params.proj = dirLight.cascades[0].proj * view.GetInverseViewMatrix();
        for (int i = 1; i < DirectionalLight::kNumCascades; ++i)
        {
            auto& cascade = dirLight.cascades[i];
            // Multiply to transition from 0->1 in cascade overlap regions
            float factor = 1.0f / (dirLight.cascades[i - 1].end - cascade.start);
            params.plane[i - 1] = factor * cascade.frontPlane;
            params.scale[i - 1] = Vector4(cascade.scale);
            params.offset[i - 1] = Vector4(cascade.offset);
        }
        ctx.Uniform("u_DirectionalLight"_id, params);

        ctx.BindTexture("u_ShadowMap"_id, momentShadows);

        // Bind environment IBL parameters
        auto& env = view.GetScene().environment;
        ctx.BindTexture("u_EnvIrradiance"_id, env.irradianceMap);
        ctx.BindTexture("u_EnvSpecular"_id, env.specularMap);
        ctx.BindTexture("u_BRDF"_id, env.BRDF);
        ctx.BindTexture("u_ScreenAO"_id, screenAO);
        ctx.BindTexture("u_ScreenReflections"_id, screenReflections);

        ctx.BindTexture("u_BaseColor"_id, gBuffer.baseColor);
        ctx.BindTexture("u_Normal"_id, gBuffer.normals);
        ctx.BindTexture("u_MetalRough"_id, gBuffer.metalRoughness);
        ctx.BindTexture("u_Depth"_id, depth);
        ctx.BindTexture("u_Emissive"_id, gBuffer.emissive);

        ctx.BindBuffer(quad->vertexBuffer);
        ctx.BindBuffer(quad->indexBuffer);
        ctx.Draw(quad->numIndices);

        ctx.EndRenderPass();
    });

    renderer.AddPass("Skybox", [=](Context& ctx, View& view)
    {
        ctx.BeginRenderPass(framebuffer);

        ctx.BindPipeline(skyboxPipeline);
        view.BindUniforms(ctx);

        ctx.BindTexture("u_Skybox"_id, view.GetScene().environment.cubeMap);

        ctx.BindBuffer(cube->indexBuffer);
        ctx.BindBuffer(cube->vertexBuffer);
        ctx.Draw(cube->numIndices);

        ctx.EndRenderPass();
    });

}

}
