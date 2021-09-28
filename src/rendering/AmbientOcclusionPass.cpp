#include "AmbientOcclusionPass.hpp"

#include "device/Context.hpp"

namespace lucent
{

Texture* AddGTAOPass(Renderer& renderer, GBuffer gBuffer, Texture* hiZ)
{
    auto& settings = renderer.GetSettings();
    uint32 width = settings.viewportWidth / 2;
    uint32 height = settings.viewportHeight / 2;

    auto aoMap = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = height, .format = TextureFormat::kR32F, .usage = TextureUsage::kReadWrite
    });

    auto computeGTAO = renderer.AddPipeline(PipelineSettings{
        .shaderName = "GTAO.shader",
        .type = PipelineType::kCompute
    });

    renderer.AddPass("Compute GTAO", [=](Context& ctx, Scene& scene)
    {
        auto& camera = scene.mainCamera.Get<Camera>();
        auto proj = camera.GetProjectionMatrix();

        ctx.BindPipeline(computeGTAO);
        ctx.BindTexture("u_Depth"_id, hiZ, 0);
        ctx.BindTexture("u_Normals"_id, gBuffer.normals);
        ctx.BindImage("u_AO"_id, aoMap);

        auto w = (float)width;
        auto h = (float)height;

        ctx.Uniform("u_ScreenToViewFactors"_id, Vector3(
            2.0f * (1.0f / proj(0, 0)) / w,
            2.0f * (1.0f / proj(1, 1)) / h,
            proj(2, 3)));

        ctx.Uniform("u_ScreenToViewOffsets"_id, Vector3(
            w / 2.0f,
            h / 2.0f,
            proj(2, 2)));

        ctx.Uniform("u_InvScreenSize"_id, Vector2(1.0f / w, 1.0f / h));

        float factor = (proj(1, 1) * h) / 2.0f;
        ctx.Uniform("u_MaxPixelRadiusZ"_id, factor);

        /******* DEBUG ********/
        auto& input = ctx.GetDevice()->m_Input->GetState();

        struct MousePos
        {
            uint32 x;
            uint32 y;
        };
        auto mousePos = MousePos{ (uint32)Round(input.cursorPos.x), (uint32)Round(input.cursorPos.y) };

        //ctx.BindBuffer("DebugShapes"_id, m_DebugShapeBuffer);
        ctx.Uniform("u_DebugCursorPos"_id, mousePos);
        //ctx.Uniform("u_ViewToWorld"_id, invView);

        /**********************/
        ctx.Dispatch(width, height, 1);

        // Spatial blur
        // Temporal blur
    });

    return aoMap;
}

}
