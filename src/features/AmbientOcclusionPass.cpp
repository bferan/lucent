#include "AmbientOcclusionPass.hpp"

#include "device/Context.hpp"

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

    auto debugShapesBuffer = renderer.GetDebugShapesBuffer();

    renderer.AddPass("Compute GTAO", [=](Context& ctx, Scene& scene)
    {
        auto& camera = scene.mainCamera.Get<Camera>();
        auto proj = camera.GetProjectionMatrix();
        auto invView = camera.GetInverseViewMatrix(scene.mainCamera.GetPosition());

        ctx.BindPipeline(computeGTAO);
        ctx.BindTexture("u_Depth"_id, hiZ, 0);
        ctx.BindTexture("u_Normals"_id, gBuffer.normals);
        ctx.BindImage("u_AO"_id, aoResult);

        ctx.Uniform("u_ScreenToView"_id, Vector4(
            2.0f * (1.0f / proj(0, 0)),
            2.0f * (1.0f / proj(1, 1)),
            proj(2, 2), proj(2, 3)));

        ctx.Uniform("u_AspectRatio"_id, proj(1, 1) / proj(0, 0));
        ctx.Uniform("u_ViewToScreenZ"_id, 0.5f * proj(1, 1));

        /******* DEBUG ********/
        auto& input = ctx.GetDevice()->m_Input->GetState();

        struct MousePos
        {
            uint32 x;
            uint32 y;
        };
        auto mousePos = MousePos{ (uint32)Round(input.cursorPos.x) / 2, (uint32)Round(input.cursorPos.y) / 2 };

        ctx.BindBuffer("DebugShapes"_id, debugShapesBuffer);
        ctx.Uniform("u_DebugCursorPos"_id, mousePos);
        ctx.Uniform("u_ViewToWorld"_id, invView);

        /**********************/
        auto[numX, numY] = settings.GetNumGroups(width, height);
        ctx.Dispatch(numX, numY, 1);
    });

    renderer.AddPass("Denoise GTAO", [=](Context& ctx, Scene& scene)
    {
        ctx.BindPipeline(denoiseGTAO);

        ctx.BindTexture("u_AORaw"_id, aoResult);
        ctx.BindImage("u_AODenoised"_id, aoDenoised);

        auto[numX, numY] = settings.GetNumGroups(width, height);
        ctx.Dispatch(numX, numY, 1);
    });

    return aoDenoised;
}

}
