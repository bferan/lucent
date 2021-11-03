#include "LightingPass.hpp"

#include "rendering/Geometry.hpp"
#include "device/Context.hpp"

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
        .colorTextures = { sceneRadiance }
    });

    auto lightingPipeline = renderer.AddPipeline(PipelineSettings{
        .shaderName = "LightingPass.shader",
        .framebuffer = framebuffer
    });

    renderer.AddPass("Lighting", [=](Context& ctx, Scene& scene)
    {
        auto& camera = scene.mainCamera.Get<Camera>();
        auto view = camera.GetViewMatrix(scene.mainCamera.GetPosition());
        auto proj = camera.GetProjectionMatrix();
        auto invView = camera.GetInverseViewMatrix(scene.mainCamera.GetPosition());

        ctx.BeginRenderPass(framebuffer);
        ctx.Clear();

        ctx.BindPipeline(lightingPipeline);

        // Bind directional light parameters
        auto light = scene.mainDirectionalLight;
        auto& dirLight = light.Get<DirectionalLight>();

        DirectionalLightParams params{};
        params.color = dirLight.color;
        params.direction = view * Vector4(light.Get<Transform>().TransformDirection(Vector3::Forward()), 0.0);
        params.proj = dirLight.cascades[0].proj * camera.GetInverseViewMatrix(scene.mainCamera.GetPosition());
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
        auto& env = scene.environment;
        ctx.BindTexture("u_EnvIrradiance"_id, env.irradianceMap);
        ctx.BindTexture("u_EnvSpecular"_id, env.specularMap);
        ctx.BindTexture("u_BRDF"_id, env.BRDF);
        ctx.BindTexture("u_ScreenAO"_id, screenAO);
        ctx.BindTexture("u_ScreenReflections"_id, screenReflections);

        ctx.BindTexture("u_BaseColor"_id, gBuffer.baseColor);
        ctx.BindTexture("u_Normal"_id, gBuffer.normals);
        ctx.BindTexture("u_MetalRough"_id, gBuffer.metalRoughness);
        ctx.BindTexture("u_Depth"_id, depth);

        ctx.Uniform("u_ScreenToView"_id, Vector4(
            2.0f * (1.0f / proj(0, 0)),
            2.0f * (1.0f / proj(1, 1)),
            proj(2, 2), proj(2, 3)));

        ctx.Uniform("u_ViewInv"_id, invView);

        ctx.BindBuffer(g_Quad.indices);
        ctx.BindBuffer(g_Quad.vertices);
        ctx.Draw(g_Quad.numIndices);

        ctx.EndRenderPass();
    });
}

}
