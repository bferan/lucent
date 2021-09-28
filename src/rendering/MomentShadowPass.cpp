#include "MomentShadowPass.hpp"

#include "device/Context.hpp"
#include "rendering/Geometry.hpp"

namespace lucent
{

static void CalculateCascades(Scene& scene)
{
    auto& viewOrigin = scene.mainCamera.Get<Transform>();
    auto& view = scene.mainCamera.Get<Camera>();

    auto& lightOrigin = scene.mainDirectionalLight.Get<Transform>();
    auto& light = scene.mainDirectionalLight.Get<DirectionalLight>();

    auto camToWorld =
        Matrix4::Translation(viewOrigin.position) *
            Matrix4::RotationY(view.yaw) *
            Matrix4::RotationX(view.pitch) *
            Matrix4::RotationX(kPi); // Flip axes

    auto worldToLight =
        Matrix4::RotationX(kPi) * // Flip axes
            Matrix4::Rotation(lightOrigin.rotation.Inverse());

    auto lightToWorld =
        Matrix4::Rotation(lightOrigin.rotation) *
            Matrix4::RotationX(kPi); // Flip axes

    auto camToLight = worldToLight * camToWorld;

    auto focalLength = 1.0f / Tan(view.verticalFov / 2.0f);

    for (auto& cascade: light.cascades)
    {
        // Determine cascade bounds based on max possible diameter of cascade frustum (ceil to int)
        auto bottomRight = Vector3(view.aspectRatio, 1.0f, focalLength);
        auto topLeft = Vector3(-view.aspectRatio, -1.0f, focalLength);

        auto back = (view.near + cascade.start) / focalLength;
        auto front = (view.near + cascade.end) / focalLength;

        auto backBottomRight = back * bottomRight;
        auto frontBottomRight = front * bottomRight;
        auto frontTopLeft = front * topLeft;

        auto diameter = Ceil(Max(
            (frontTopLeft - backBottomRight).Length(),
            (frontTopLeft - frontBottomRight).Length()));

        // Determine texel size in world space
        cascade.worldSpaceTexelSize = diameter / (float)DirectionalLight::kMapWidth;

        // Find position of cam for light to the nearest texel size multiple for stability
        auto minPos = Vector3::Infinity();
        auto maxPos = Vector3::NegativeInfinity();

        for (auto dist: { cascade.start, cascade.end })
        {
            for (auto x: { -view.aspectRatio, view.aspectRatio })
            {
                for (auto y: { -1.0f, 1.0f })
                {
                    auto camPos = ((view.near + dist) / focalLength) * Vector3(x, y, focalLength);
                    auto lightPos = Vector3(camToLight * Vector4(camPos, 1.0f));

                    minPos = Min(minPos, lightPos);
                    maxPos = Max(maxPos, lightPos);
                }
            }
        }
        auto texelSize = cascade.worldSpaceTexelSize;
        auto alignedX = Floor((0.5f * (minPos.x + maxPos.x)) / texelSize) * texelSize;
        auto alignedY = Floor((0.5f * (minPos.y + maxPos.y)) / texelSize) * texelSize;

        cascade.pos = Vector3(lightToWorld * Vector4(alignedX, alignedY, minPos.z, 1.0));
        cascade.width = diameter;
        cascade.depth = maxPos.z - minPos.z;

        cascade.proj = Matrix4::Orthographic(cascade.width, cascade.width, cascade.depth) *
            Matrix4::RotationX(kPi) *
            Matrix4::Rotation(lightOrigin.rotation.Inverse()) *
            Matrix4::Translation(-cascade.pos);

        // Compute offset and scale to convert cascade-0 coords into this cascade's coord space
        auto dPos = light.cascades[0].pos - cascade.pos;

        auto rot = Matrix4::Rotation(lightOrigin.rotation.Inverse());
        dPos = Vector3(rot * Vector4(dPos, 1.0f));
        dPos *= Vector3(2.0f / cascade.width, -2.0f / cascade.width, -1.0f / cascade.depth);

        cascade.offset = dPos;

        auto dWidth = light.cascades[0].width / cascade.width;
        auto dDepth = light.cascades[0].depth / cascade.depth;
        cascade.scale = Vector3(dWidth, dWidth, dDepth);

        // Compute plane at front of this cascade
        auto camNormal = Vector3(0.0, 0.0, 1.0);
        auto camPos = Vector3::Zero();
        camPos += (view.near + cascade.start) * camNormal;

        auto camD = camNormal.Dot(camPos);
        cascade.frontPlane = Vector4(camNormal, -camD);
    }

    // TODO: Adjust cascade index so that all cascades in a quad use the same cascade?
}

Texture* AddMomentShadowPass(Renderer& renderer)
{
    auto& settings = renderer.GetSettings();

    uint32 width = 2048;
    uint32 samples = 8;
    uint32 numCascades = 4;
    uint32 levels = 6;

    auto momentMap = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = width,
        .levels = levels,
        .layers = numCascades,
        .format = TextureFormat::kRGBA32F,
        .shape = TextureShape::k2DArray,
        .addressMode = TextureAddressMode::kClampToBorder
    });

    auto tempDepth = renderer.AddRenderTarget(TextureSettings{
        .width = width, .height = width,
        .format = TextureFormat::kDepth16U,
        .usage = TextureUsage::kDepthAttachment
    });

    std::vector<Texture*> depthTextures(numCascades);
    std::vector<Framebuffer*> depthFramebuffers(numCascades);
    std::vector<Framebuffer*> momentMapLayers(numCascades);

    for (int i = 0; i < numCascades; ++i)
    {
        auto depth = depthTextures[i] = renderer.AddRenderTarget(TextureSettings{
            .width = width, .height = width,
            .samples = samples,
            .format = TextureFormat::kDepth16U,
            .usage = TextureUsage::kDepthAttachment
        });

        depthFramebuffers[i] = renderer.AddFramebuffer(FramebufferSettings{
            .depthTexture = depth
        });

        momentMapLayers[i] = renderer.AddFramebuffer(FramebufferSettings{
            .colorTextures = { momentMap },
            .colorLayer = i,
            .depthTexture = tempDepth
        });
    }

    auto depthOnly = renderer.AddPipeline(PipelineSettings{
        .shaderName = "DepthOnly.shader",
        .framebuffer = depthFramebuffers.back(),
        .depthClampEnable = true
    });

    auto resolveDepth = renderer.AddPipeline(PipelineSettings{
        .shaderName = "ResolveMoments.shader",
        .framebuffer = momentMapLayers.back()
    });

    renderer.AddPass("Shadow map render depth MS", [=](Context& ctx, Scene& scene)
    {
        CalculateCascades(scene);
        auto& cascades = scene.mainDirectionalLight.Get<DirectionalLight>().cascades;

        // Render depth to the moment MS depth textures
        for (int i = 0; i < numCascades; ++i)
        {
            ctx.BeginRenderPass(depthFramebuffers[i]);
            ctx.Clear();

            auto& cascade = cascades[i];
            ctx.BindPipeline(depthOnly);
            scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
            {
                for (auto idx: instance.meshes)
                {
                    auto& mesh = scene.meshes[idx];
                    auto mvp = cascade.proj * local.model;
                    ctx.Uniform("u_MVP"_id, mvp);

                    ctx.BindBuffer(mesh.vertexBuffer);
                    ctx.BindBuffer(mesh.indexBuffer);
                    ctx.Draw(mesh.numIndices);
                }
            });
            ctx.EndRenderPass();
        }
    });

    renderer.AddPass("Shadow map resolve depth", [=](Context& ctx, Scene& scene)
    {
        // Calculate moments from depth values using custom resolve
        for (int i = 0; i < numCascades; ++i)
        {
            auto depthTexture = depthTextures[i];
            auto framebuffer = momentMapLayers[i];

            ctx.BeginRenderPass(framebuffer);

            ctx.BindPipeline(resolveDepth);
            ctx.BindTexture("u_Depth"_id, depthTexture);

            ctx.BindBuffer(g_Quad.vertices);
            ctx.BindBuffer(g_Quad.indices);
            ctx.Draw(g_Quad.numIndices);

            ctx.EndRenderPass();
        }
        ctx.GenerateMips(momentMap);
    });

    // TODO: Blur moment map with gaussian kernel

    return momentMap;
}

}
