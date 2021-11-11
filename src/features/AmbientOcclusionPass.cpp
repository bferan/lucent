#include "AmbientOcclusionPass.hpp"

#include "device/Context.hpp"
#include "scene/Camera.hpp"

namespace lucent
{

Texture* AddGTAOPass(Renderer& renderer, GBuffer gBuffer, Texture* hiZ)
{
    auto& settings = renderer.GetSettings();
    uint32 width = settings.viewportWidth / 2;
    uint32 height = settings.viewportHeight / 2;

    auto aoResult = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height, .format = TextureFormat::kRG32F, .usage = TextureUsage::kReadWrite
    });

    auto aoDenoised = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height, .format = TextureFormat::kR32F, .usage = TextureUsage::kReadWrite
    });

    auto computeGTAO = renderer.AddPipeline(PipelineSettings{
        .shaderName = "GTAO.shader",
        .type = PipelineType::kCompute
    });

    auto denoiseGTAO = renderer.AddPipeline(PipelineSettings{
        .shaderName = "GTAODenoise.shader",
        .type = PipelineType::kCompute
    });

    renderer.AddPass("Compute GTAO", [=, &settings](Context& ctx, View& view)
    {
        ctx.BindPipeline(computeGTAO);
        view.BindUniforms(ctx);

        ctx.BindTexture("u_Depth"_id, hiZ, 0);
        ctx.BindTexture("u_Normals"_id, gBuffer.normals);
        ctx.BindImage("u_AO"_id, aoResult);

        ctx.Uniform("u_ViewToScreenZ"_id, 0.5f * view.GetProjectionMatrix()(1, 1));

        auto[numX, numY] = settings.ComputeGroupCount(width, height);
        ctx.Dispatch(numX, numY, 1);
    });

    renderer.AddPass("Denoise GTAO", [=, &settings](Context& ctx, View& view)
    {
        ctx.BindPipeline(denoiseGTAO);

        ctx.BindTexture("u_AORaw"_id, aoResult);
        ctx.BindImage("u_AODenoised"_id, aoDenoised);

        auto[numX, numY] = settings.ComputeGroupCount(width, height);
        ctx.Dispatch(numX, numY, 1);
    });

    return aoDenoised;
}

}
