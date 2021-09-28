#include "DebugOverlayPass.hpp"

#include "device/Context.hpp"
#include "rendering/Geometry.hpp"

namespace lucent
{

void AddDebugOverlayPass(Renderer& renderer, DebugConsole* console, Texture* output)
{
    auto& settings = renderer.GetSettings();
    console->SetScreenSize(settings.viewportWidth, settings.viewportHeight);

    auto overlayFramebuffer = renderer.AddFramebuffer(FramebufferSettings{
        .colorTextures = { output }
    });

    auto debugTextShader = renderer.AddPipeline(PipelineSettings{
        .shaderName = "DebugFont.shader", .framebuffer = overlayFramebuffer, .depthTestEnable = false
    });

    renderer.AddPass("Debug console", [=](Context& ctx, Scene& scene)
    {
        ctx.BeginRenderPass(overlayFramebuffer);

        ctx.BindPipeline(debugTextShader);
        console->RenderText(ctx);

        ctx.EndRenderPass();
    });

#if 0
    //   m_DebugShapeBuffer = m_Device->CreateBuffer(BufferType::kStorage, sizeof(DebugShapeBuffer));
    //  m_DebugShapeBuffer->Clear(sizeof(DebugShapeBuffer), 0);
    //  m_DebugShapes = static_cast<DebugShapeBuffer*>(m_DebugShapeBuffer->Map());

    auto debugShapePipeline = renderer.AddPipeline(PipelineSettings{
        .shaderName = "DebugShape.shader",
    });

    renderer.AddPass("Debug overlay", [=](Context& ctx, Scene& scene)
    {
        ctx.BindPipeline(m_DebugShapePipeline);
        for (int i = 0; i < m_DebugShapes->numShapes; ++i)
        {
            auto& shape = m_DebugShapes->shapes[i];

            auto mvp = proj * view *
                Matrix4::Translation(shape.srcPos) *
                Matrix4::Scale(shape.r, shape.r, shape.r);

            ctx.Uniform("u_MVP"_id, mvp);
            ctx.Uniform("u_Color"_id, shape.color);

            ctx.BindBuffer(g_Sphere.vertices);
            ctx.BindBuffer(g_Sphere.indices);
            ctx.Draw(g_Sphere.numIndices);
        }

        console->Render(ctx);
    });
#endif
}

}
