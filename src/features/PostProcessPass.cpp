#include "PostProcessPass.hpp"

#include "device/Context.hpp"

namespace lucent
{

static Texture* AddBloomPass(Renderer& renderer, Texture* sceneRadiance)
{
    auto& settings = renderer.GetSettings();

    // Skip 1, 2 and 4 pixel wide bloom mips
    constexpr uint32 kSkipMips = 3u;
    auto bloomMips = (uint32)Floor(Log2((float)Max(settings.viewportWidth, settings.viewportHeight))) + 1 - kSkipMips;
    bloomMips = Max(bloomMips, 1u);

    auto bloomMipSettings = TextureSettings{
        .width = settings.viewportWidth,
        .height = settings.viewportHeight,
        .levels = bloomMips,
        .format = TextureFormat::kRGBA32F,
        .addressMode = TextureAddressMode::kClampToEdge,
        .usage = TextureUsage::kReadWrite
    };
    auto bloomDownsampleMips = renderer.AddRenderTarget(bloomMipSettings);
    auto bloomUpsampleMips = renderer.AddRenderTarget(bloomMipSettings);

    auto bloomDownsample = renderer.AddPipeline(PipelineSettings{
        .shaderName = "Bloom.shader",
        .type = PipelineType::kCompute
    });
    auto bloomDownsampleWeighted = renderer.AddPipeline(PipelineSettings{
        .shaderName = "Bloom.shader",
        .shaderDefines = { "AVERAGE KarisAverage" },
        .type = PipelineType::kCompute
    });

    auto bloomUpsample = renderer.AddPipeline(PipelineSettings{
        .shaderName = "Bloom.shader",
        .shaderDefines = { "BLOOM_UPSAMPLE" },
        .type = PipelineType::kCompute
    });

    renderer.AddPass("Bloom", [=, &settings](Context& ctx, View& view)
    {
        auto[srcWidth, srcHeight] = sceneRadiance->GetSize();
        ctx.CopyTexture(sceneRadiance, 0, 0, bloomDownsampleMips, 0, 0, srcWidth, srcHeight);

        // Progressively downsample
        for (int srcMip = 0; srcMip < bloomMips - 1; ++srcMip)
        {
            ctx.BindPipeline((srcMip == 0) ? bloomDownsampleWeighted : bloomDownsample);
            ctx.BindTexture("u_Input"_id, bloomDownsampleMips, srcMip);
            ctx.BindImage("u_Output"_id, bloomDownsampleMips, srcMip + 1);

            auto[width, height] = bloomDownsampleMips->GetMipSize(srcMip + 1);
            auto[groupsX, groupsY] = settings.ComputeGroupCount(width, height);
            ctx.Dispatch(groupsX, groupsY, 1);
        }

        // Progressively upsample and combine
        ctx.BindPipeline(bloomUpsample);
        auto input = bloomDownsampleMips;
        for (int srcMip = (int)bloomMips - 1; srcMip > 0; --srcMip)
        {
            ctx.BindTexture("u_Input"_id, input, srcMip);
            ctx.BindTexture("u_InputHigh"_id, bloomDownsampleMips, srcMip - 1);
            ctx.BindImage("u_Output"_id, bloomUpsampleMips, srcMip - 1);

            ctx.Uniform("u_FilterRadius"_id, 0.05f);
            ctx.Uniform("u_Strength"_id, 0.35f);

            auto[width, height] = bloomUpsampleMips->GetMipSize(srcMip - 1);
            auto[groupsX, groupsY] = settings.ComputeGroupCount(width, height);
            ctx.Dispatch(groupsX, groupsY, 1);

            // Rest of inputs are from upsample chain
            input = bloomUpsampleMips;
        }
    });
    return bloomUpsampleMips;
}

Texture* AddPostProcessPass(Renderer& renderer, Texture* sceneRadiance)
{
    auto& settings = renderer.GetSettings();

    // TODO: Expose as settings
    const float kBloomStrength = 0.35f;
    const float kVignetteIntensity = 15.0f;
    const float kVignetteExtent = 0.25f;

    auto bloomOutput = AddBloomPass(renderer, sceneRadiance);

    auto output = renderer.AddRenderTarget(TextureSettings{
        .width = settings.viewportWidth,
        .height = settings.viewportHeight,
        .format = TextureFormat::kRGBA8,
        .usage = TextureUsage::kReadWrite
    });

    auto computeOutput = renderer.AddPipeline(PipelineSettings{
        .shaderName = "PostProcessOutput.shader",
        .shaderDefines = { "PP_BLOOM", "PP_TONEMAP", "PP_VIGNETTE" },
        .type = PipelineType::kCompute
    });

    renderer.AddPass("Post-process Output", [=, &settings](Context& ctx, View& view)
    {
        ctx.BindPipeline(computeOutput);
        ctx.BindTexture("u_Input"_id, sceneRadiance);
        ctx.BindTexture("u_Bloom"_id, bloomOutput, 0);
        ctx.BindImage("u_Output"_id, output);

        ctx.Uniform("u_BloomStrength"_id, kBloomStrength);

        ctx.Uniform("u_VignetteIntensity"_id, kVignetteIntensity);
        ctx.Uniform("u_VignetteExtent"_id, kVignetteExtent);

        auto[numX, numY] = settings.ComputeGroupCount(settings.viewportWidth, settings.viewportHeight);
        ctx.Dispatch(numX, numY, 1);
    });

    return output;
}

}
