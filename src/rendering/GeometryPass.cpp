#include "GeometryPass.hpp"

#include "device/Context.hpp"

namespace lucent
{

GBuffer AddGeometryPass(Renderer& renderer, const RenderSettings& settings)
{
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

    renderer.AddPass("Geometry pass", [=](Context& ctx)
    {
        auto& scene = settings.scene;

        ctx.BeginRenderPass(framebuffer);
        ctx.Clear();

        ctx.BindPipeline(renderGeometry);

        scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
        {
            for (auto idx: instance.meshes)
            {
                auto& mesh = scene.meshes[idx];
                auto& material = scene.materials[mesh.materialIdx];

                auto view = Matrix4::Identity();
                auto proj = Matrix4::Identity();

                auto mv = view * local.model;
                auto mvp = proj * mv;

                // Bind globals
                ctx.Uniform("u_MVP"_id, mvp);
                ctx.Uniform("u_MV"_id, mv);

                // Bind material data
                ctx.BindTexture("u_BaseColor"_id, material.baseColorMap);
                ctx.BindTexture("u_MetalRoughness"_id, material.metalRough);
                ctx.BindTexture("u_Normal"_id, material.normalMap);

                ctx.Uniform("u_BaseColorFactor"_id, material.baseColorFactor);
                ctx.Uniform("u_MetallicFactor"_id, material.metallicFactor);
                ctx.Uniform("u_RoughnessFactor"_id, material.roughnessFactor);

                ctx.BindBuffer(mesh.vertexBuffer);
                ctx.BindBuffer(mesh.indexBuffer);
                ctx.Draw(mesh.numIndices);
            }
        });

        ctx.EndRenderPass();
    });

    return gBuffer;
}

Texture* AddGenerateHiZPass(Renderer& renderer, const RenderSettings& settings, Texture* depthTexture)
{
    uint32 baseWidth = depthTexture->width;
    uint32 baseHeight = depthTexture->height;

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

    renderer.AddPass("Generate Hi-Z", [=](Context& ctx)
    {
        // Copy depth texture to level 0 of color mip pyramid
        ctx.CopyTexture(depthTexture, 0, 0, buffer, 0, baseWidth, baseHeight);
        ctx.CopyTexture(buffer, 0, hiZ, 0, 0, baseWidth, baseHeight);

        // Progressively render to lower mip levels
        ctx.BindPipeline(generateHiZ);
        uint32 width = baseWidth;
        uint32 height = baseHeight;

        for (int level = 1; level < hiZ->levels; ++level)
        {
            width = Max(width / 2u, 1u);
            height = Max(height / 2u, 1u);

            ctx.BindTexture("u_Input"_id, hiZ, level - 1);
            ctx.BindImage("u_Output"_id, hiZ, level);

            ctx.Dispatch(baseWidth, baseHeight, 1);
        }
    });

    return hiZ;
}


}
