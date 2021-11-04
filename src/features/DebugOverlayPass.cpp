#include "DebugOverlayPass.hpp"

#include "device/Context.hpp"
#include "rendering/Geometry.hpp"
#include "scene/Camera.hpp"

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

    auto debugShapeShader = renderer.AddPipeline(PipelineSettings{
        .shaderName = "DebugShape.shader", .framebuffer = overlayFramebuffer, . depthTestEnable = false
    });

    auto debugShapes = (DebugShapeBuffer*)renderer.GetDebugShapesBuffer()->Map();

    renderer.AddPass("Debug overlay", [=](Context& ctx, Scene& scene)
    {
        auto& camera = scene.mainCamera.Get<Camera>();
        auto view = camera.GetViewMatrix(scene.mainCamera.GetPosition());
        auto proj = camera.GetProjectionMatrix();

        ctx.GetDevice()->WaitIdle();

        ctx.BeginRenderPass(overlayFramebuffer);

        ctx.BindPipeline(debugShapeShader);
        for (int i = 0; i < debugShapes->numShapes; ++i)
        {
            auto& shape = debugShapes->shapes[i];

            //
            auto mvp = proj * view *
                Matrix4::Translation(Vector3(shape.srcPos)) *
                Matrix4::Scale(shape.radius, shape.radius, shape.radius);

            ctx.Uniform("u_MVP"_id, mvp);
            ctx.Uniform("u_Color"_id, shape.color);

            ctx.BindBuffer(g_Sphere.vertices);
            ctx.BindBuffer(g_Sphere.indices);
            ctx.Draw(g_Sphere.numIndices);
        }

        ctx.BindPipeline(debugTextShader);
        console->RenderText(ctx);

        ctx.EndRenderPass();

        debugShapes->numShapes = 0;
    });
}

}