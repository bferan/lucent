#include "ScreenSpaceReflectionsPass.hpp"

namespace lucent
{

Texture* AddScreenSpaceReflectionsPass(Renderer& renderer, GBuffer gBuffer, Texture* minZ, Texture* prevColor)
{
    auto& settings = renderer.GetSettings();

    uint32 width = settings.viewportWidth;
    uint32 height = settings.viewportHeight;

    // Trace rays
    auto rayHits = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height,
        .format = TextureFormat::kRG32F,
        .usage = TextureUsage::kReadWrite
    });

    auto traceReflections = renderer.AddPipeline(PipelineSettings{
        .shaderName = "SSRTraceMinZ.shader", .type = PipelineType::kCompute
    });

    renderer.AddPass("SSR trace rays", [=](Context& ctx, View& view)
    {
        ctx.BindPipeline(traceReflections);
        view.BindUniforms(ctx);

        ctx.BindTexture("u_MinZ"_id, minZ);
        ctx.BindTexture("u_Normals"_id, gBuffer.normals);
        ctx.BindImage("u_Result"_id, rayHits);

        auto[numX, numY] = settings.GetNumGroups(width, height);
        ctx.Dispatch(numX, numY, 1);
    });

    // Scene blurring
    auto levels = (uint32)Floor(Log2((float)Max(width, height))) + 1;

    auto convolvedScene = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height, .levels = levels,
        .format = TextureFormat::kRGBA32F,
        .usage = TextureUsage::kReadWrite,
    });

    auto tempTargetSettings = TextureSettings{
        .width = width / 2, .height = height / 2, .levels = levels - 1,
        .format = TextureFormat::kRGBA32F,
        .addressMode = TextureAddressMode::kClampToEdge,
        .usage = TextureUsage::kReadWrite,
    };
    auto downsampleTarget = renderer.AddRenderTarget(tempTargetSettings);
    auto blurTarget = renderer.AddRenderTarget(tempTargetSettings);

    auto blurHorizontal = renderer.AddPipeline(PipelineSettings{
        .shaderName = "SSRConvolve.shader", .shaderDefines = { "BLUR_HORIZONTAL" }, .type = PipelineType::kCompute
    });
    auto blurVertical = renderer.AddPipeline(PipelineSettings{
        .shaderName = "SSRConvolve.shader", .shaderDefines = { "BLUR_VERTICAL" }, .type = PipelineType::kCompute
    });

    renderer.AddPass("SSR pre-convolve", [=](Context& ctx, View& view)
    {
        ctx.BlitTexture(prevColor, 0, 0, convolvedScene, 0, 0);

        for (int mip = 0; mip < levels - 1; ++mip)
        {
            // Downsample
            ctx.BlitTexture(convolvedScene, 0, 0, downsampleTarget, 0, mip);

            auto[mipWidth, mipHeight] = downsampleTarget->GetMipSize(mip);
            auto[groupsX, groupsY] = settings.GetNumGroups(mipWidth, mipHeight);

            // Horizontal blur
            ctx.BindPipeline(blurHorizontal);
            ctx.BindTexture("u_Input"_id, downsampleTarget, mip);
            ctx.BindImage("u_Output"_id, blurTarget, mip);
            ctx.Dispatch(groupsX, groupsY, 1);

            // Vertical blur & output
            ctx.BindPipeline(blurVertical);
            ctx.BindTexture("u_Input"_id, blurTarget, mip);
            ctx.BindImage("u_Output"_id, convolvedScene, mip + 1);
            ctx.Dispatch(groupsX, groupsY, 1);
        }
    });

    // Resolve/reproject
    auto resolvedReflections = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height,
        .format = TextureFormat::kRGBA32F,
        .usage = TextureUsage::kReadWrite
    });

    auto resolveReflections = renderer.AddPipeline(PipelineSettings{
        .shaderName = "SSRResolveReflections.shader", .type = PipelineType::kCompute
    });

    renderer.AddPass("SSR resolve reflections", [=](Context& ctx, View& view)
    {
        ctx.BindPipeline(resolveReflections);
        view.BindUniforms(ctx);

        ctx.BindTexture("u_Rays"_id, rayHits);
        ctx.BindTexture("u_ConvolvedScene"_id, convolvedScene);
        ctx.BindTexture("u_Depth"_id, minZ);
        ctx.BindTexture("u_MetalRoughness"_id, gBuffer.metalRoughness);
        ctx.BindImage("u_Result"_id, resolvedReflections);

        auto[numX, numY] = settings.GetNumGroups(width, height);
        ctx.Dispatch(numX, numY, 1);
    });

    return resolvedReflections;
}

}
