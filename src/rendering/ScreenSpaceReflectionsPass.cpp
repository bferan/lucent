#include "ScreenSpaceReflectionsPass.hpp"

#include "device/Context.hpp"

namespace lucent
{

static std::vector<Vector2> GenerateBlueNoise(int size)
{
    constexpr int kIterations = 100000;
    constexpr int kRadius = 4;

    constexpr float kSigmaDistSqr = 2.1f * 2.1f;
    constexpr float kSigmaValueSqr = 1.0f;

    // Fill with white noise
    std::vector<Vector2> noise(size * size);
    for (auto& i: noise)
    {
        i = { RandF(), RandF() };
    }

    // Energy function to minimise
    auto energy = [&](int x, int y)
    {
        float total = 0.0f;
        auto value = noise[x + size * y];
        for (int dx = -kRadius; dx <= kRadius; ++dx)
        {
            for (int dy = -kRadius; dy <= kRadius; ++dy)
            {
                if (dx == 0 && dy == 0)
                    continue;

                auto distSqr = float(dx * dx + dy * dy);

                int sx = (x + dx + size) % size;
                int sy = (y + dy + size) % size;
                auto sample = noise[sx + sy * size];

                total += Exp(
                    -distSqr / kSigmaDistSqr
                        - (sample - value).Length() / kSigmaValueSqr
                );
            }
        }
        return total;
    };

    // Perform simulated annealing
    for (int i = 0; i < kIterations; ++i)
    {
        int x1 = Rand() % size;
        int y1 = Rand() % size;
        int x2 = Rand() % size;
        int y2 = Rand() % size;

        if (x1 == x2 && y1 == y2)
            continue;

        auto prevEnergy = energy(x1, y1) + energy(x2, y2);
        std::swap(noise[x1 + y1 * size], noise[x2 + y2 * size]);
        auto nextEnergy = energy(x1, y1) + energy(x2, y2);

        // Swap back
        if (nextEnergy > prevEnergy)
            std::swap(noise[x1 + y1 * size], noise[x2 + y2 * size]);
    }

    return noise;
}

Texture* AddScreenSpaceReflectionsPass(Renderer& renderer, GBuffer gBuffer, Texture* minZ, Texture* prevColor)
{
    auto& settings = renderer.GetSettings();

    uint32 noiseSize = 128;
    auto blueNoise = GenerateBlueNoise((int)noiseSize);
    auto blueNoiseTex = renderer.AddRenderTarget(TextureSettings{
        .width = noiseSize, .height = noiseSize,
        .format = TextureFormat::kRG32F,
        .addressMode = TextureAddressMode::kRepeat,
        .filter = TextureFilter::kNearest
    });
    blueNoiseTex->Upload(blueNoise.size(), blueNoise.data());

    uint32 width = settings.viewportWidth;
    uint32 height = settings.viewportHeight;

    auto rayHits = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height,
        .format = TextureFormat::kRGBA32F,
        .usage = TextureUsage::kReadWrite
    });

    auto resolvedReflections = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height,
        .format = TextureFormat::kRGBA32F,
        .usage = TextureUsage::kReadWrite
    });

    auto traceReflections = renderer.AddPipeline(PipelineSettings{
        .shaderName = "TraceMinZ.shader", .type = PipelineType::kCompute
    });

    auto resolveReflections = renderer.AddPipeline(PipelineSettings{
        .shaderName = "ResolveSSR.shader", .type = PipelineType::kCompute
    });

    renderer.AddPass("SSR trace rays", [=](Context& ctx, Scene& scene)
    {
        auto proj = scene.mainCamera.Get<Camera>().GetProjectionMatrix();

        ctx.BindPipeline(traceReflections);
        ctx.BindTexture("u_MinZ"_id, minZ);
        ctx.BindTexture("u_HaltonNoise"_id, blueNoiseTex);
        ctx.BindTexture("u_Normals"_id, gBuffer.normals);
        ctx.BindTexture("u_MetalRoughness"_id, gBuffer.metalRoughness);
        ctx.BindImage("u_Result"_id, rayHits);

        ctx.Uniform("u_ScreenToView"_id, Vector4(
            2.0f * (1.0f / proj(0, 0)),
            2.0f * (1.0f / proj(1, 1)),
            proj(2, 2), proj(2, 3)));

        ctx.Uniform("u_Proj"_id, proj);

        ctx.Dispatch(width, height, 1);
    });

    renderer.AddPass("SSR resolve reflections", [=](Context& ctx, Scene& scene)
    {
        auto proj = scene.mainCamera.Get<Camera>().GetProjectionMatrix();

        ctx.BindPipeline(resolveReflections);
        ctx.BindTexture("u_Rays"_id, rayHits);
        ctx.BindTexture("u_PrevColor"_id, prevColor);
        ctx.BindTexture("u_Depth"_id, minZ);
        ctx.BindTexture("u_BaseColor"_id, gBuffer.baseColor);
        ctx.BindTexture("u_Normals"_id, gBuffer.normals);
        ctx.BindTexture("u_MetalRoughness"_id, gBuffer.metalRoughness);
        ctx.BindImage("u_Result"_id, resolvedReflections);

        ctx.Uniform("u_ScreenToView"_id, Vector4(
            2.0f * (1.0f / proj(0, 0)),
            2.0f * (1.0f / proj(1, 1)),
            proj(2, 2), proj(2, 3)));

        ctx.Uniform("u_Proj"_id, proj);

        ctx.Dispatch(width, height, 1);
    });

    return resolvedReflections;
}

}
