#include "GeometryPass.hpp"

#include "device/Context.hpp"
#include "rendering/Material.hpp"
#include "scene/Camera.hpp"
#include "scene/ModelInstance.hpp"
#include "scene/Transform.hpp"

namespace lucent
{

GBuffer AddGeometryPass(Renderer& renderer)
{
    auto& settings = renderer.GetSettings();

    GBuffer gBuffer{};

    uint32 width = settings.viewportWidth;
    uint32 height = settings.viewportHeight;

    gBuffer.baseColor = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height, .format = TextureFormat::kRGBA8_sRGB
    });
    gBuffer.normals = renderer.AddRenderTarget(TextureSettings{
        .width = width, . height = height, .format = TextureFormat::kRGB10A2
    });
    gBuffer.metalRoughness = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height, .format = TextureFormat::kRG8
    });
    gBuffer.depth = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height, .format = TextureFormat::kDepth32F,
        .usage = TextureUsage::kDepthAttachment
    });

    auto framebuffer = renderer.AddFramebuffer(FramebufferSettings{
        .colorTextures = { gBuffer.baseColor, gBuffer.normals, gBuffer.metalRoughness },
        .depthTexture = gBuffer.depth
    });

    auto renderGeometry = renderer.AddPipeline(PipelineSettings{
        .shaderName = "GeometryPass.shader",
        .framebuffer = framebuffer
    });

    renderer.AddPass("Geometry pass", [=](Context& ctx, Scene& scene)
    {
        auto& camera = scene.mainCamera.Get<Camera>();
        auto view = camera.GetViewMatrix(scene.mainCamera.GetPosition());
        auto proj = camera.GetProjectionMatrix();

        ctx.BeginRenderPass(framebuffer);
        ctx.Clear();

        ctx.BindPipeline(renderGeometry);

        scene.Each<ModelInstance, Transform>([&](ModelInstance& instance, Transform& local)
        {
            for (auto& primitive: *instance.model)
            {
                auto& mesh = primitive.mesh;
                auto material = instance.material ? instance.material : primitive.material;

                auto mv = view * local.model;
                auto mvp = proj * mv;

                // Bind globals
                ctx.Uniform("u_MVP"_id, mvp);
                ctx.Uniform("u_MV"_id, mv);

                // Bind material data
                material->BindUniforms(ctx);

                ctx.BindBuffer(mesh.vertexBuffer);
                ctx.BindBuffer(mesh.indexBuffer);
                ctx.Draw(mesh.numIndices);
            }
        });

        ctx.EndRenderPass();
    });

    return gBuffer;
}

Texture* AddGenerateHiZPass(Renderer& renderer, Texture* depthTexture)
{
    auto& settings = renderer.GetSettings();

    auto [baseWidth, baseHeight] = depthTexture->GetSize();

    auto levels = (uint32)Floor(Log2((float)Max(baseWidth, baseHeight))) + 1;

    auto hiZ = renderer.AddRenderTarget(TextureSettings{
        .width = baseWidth, .height = baseHeight,
        .levels = levels,
        .format = TextureFormat::kR32F,
        .addressMode = TextureAddressMode::kClampToEdge,
        .filter = TextureFilter::kNearest,
        .usage = TextureUsage::kReadWrite
    });

    auto generateHiZ = renderer.AddPipeline(PipelineSettings{
        .shaderName = "GenerateHiZ.shader", .type = PipelineType::kCompute
    });

    auto buffer = renderer.GetTransferBuffer();

    renderer.AddPass("Generate Hi-Z", [=](Context& ctx, Scene& scene)
    {
        // Copy depth texture to level 0 of color mip pyramid
        ctx.CopyTexture(depthTexture, 0, 0, buffer, 0, baseWidth, baseHeight);
        ctx.CopyTexture(buffer, 0, hiZ, 0, 0, baseWidth, baseHeight);

        // Progressively render to lower mip levels
        ctx.BindPipeline(generateHiZ);
        uint32 width = baseWidth;
        uint32 height = baseHeight;

        for (int level = 1; level < hiZ->GetSettings().levels; ++level)
        {
            width = Max(width / 2u, 1u);
            height = Max(height / 2u, 1u);

            ctx.BindTexture("u_Input"_id, hiZ, level - 1);
            ctx.BindImage("u_Output"_id, hiZ, level);

            auto [numX, numY] = settings.GetNumGroups(width, height);
            ctx.Dispatch(numX, numY, 1);
        }
    });

    return hiZ;
}


}