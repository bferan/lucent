#include "PostProcessPass.hpp"

#include "device/Context.hpp"

namespace lucent
{

Texture* AddPostProcessPass(Renderer& renderer, Texture* sceneRadiance)
{
    auto& settings = renderer.GetSettings();

    auto output = renderer.AddRenderTarget(TextureSettings{
        .width = settings.viewportWidth,
        .height = settings.viewportHeight,
        .format = TextureFormat::kRGBA8,
        .usage = TextureUsage::kReadWrite
    });

    auto computePostProcess = renderer.AddPipeline(PipelineSettings{
        .shaderName = "PostProcess.shader", .type = PipelineType::kCompute
    });

    renderer.AddPass("Post-process", [=](Context& ctx, Scene& scene)
    {
        ctx.BindPipeline(computePostProcess);
        ctx.BindTexture("u_Input"_id, sceneRadiance);
        ctx.BindImage("u_Output"_id, output);
        ctx.Dispatch(settings.viewportWidth, settings.viewportHeight, 1);
    });

    return output;
}

}
