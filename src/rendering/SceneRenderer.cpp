#include "SceneRenderer.hpp"

#include "core/Utility.hpp"
#include "device/Context.hpp"

namespace lucent
{

SceneRenderer::SceneRenderer(Device* device)
    : m_Device(device)
{
    m_DefaultPipeline = m_Device->CreatePipeline(PipelineSettings{ .shaderName = "Standard.shader" });
    m_SkyboxPipeline = m_Device->CreatePipeline(PipelineSettings{ .shaderName = "Skybox.shader" });

    m_Context = m_Device->CreateContext();

    // Create debug console (temp)
    m_DebugConsole = std::make_unique<DebugConsole>(m_Device);

    // Moment shadows
    m_MomentMap = m_Device->CreateTexture(TextureSettings{
        .width = DirectionalLight::kMapWidth,
        .height = DirectionalLight::kMapWidth,
        .layers = DirectionalLight::kNumCascades,
        .format = TextureFormat::kRGBA32F,
        .shape = TextureShape::k2DArray,
        .addressMode = TextureAddressMode::kClampToBorder
    });

    m_MomentTempDepth = m_Device->CreateTexture(TextureSettings{
        .width = m_MomentMap->extent.width,
        .height = m_MomentMap->extent.height,
        .format = TextureFormat::kDepth16U
    });

    for (int i = 0; i < kShadowTargets; ++i)
    {
        auto depth = m_MomentDepthTextures.emplace_back(m_Device->CreateTexture(TextureSettings{
            .width = DirectionalLight::kMapWidth,
            .height = DirectionalLight::kMapWidth,
            .samples = 8,
            .format = TextureFormat::kDepth16U
        }));

        m_MomentDepthTargets.emplace_back(m_Device->CreateFramebuffer(FramebufferSettings{
            .depthTexture = depth
        }));

        m_MomentMapTargets.emplace_back(m_Device->CreateFramebuffer(FramebufferSettings{
            .colorTexture = m_MomentMap,
            .colorLayer = i,
            .depthTexture = m_MomentTempDepth
        }));
    }

    m_MomentDepthOnly = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "DepthOnly.shader",
        .framebuffer = m_MomentDepthTargets[0],
        .depthClampEnable = true
    });

    m_MomentResolveDepth = m_Device->CreatePipeline(PipelineSettings{
       .shaderName = "ResolveMoments.shader",
       .framebuffer = m_MomentMapTargets[0]
    });

}

void SceneRenderer::Render(Scene& scene)
{
    // Find first camera
    auto& transform = scene.mainCamera.Get<Transform>();
    auto& camera = scene.mainCamera.Get<Camera>();

    // Calc camera position
    auto view = Matrix4::RotationX(kPi) *
        Matrix4::RotationX(-camera.pitch) *
        Matrix4::RotationY(-camera.yaw) *
        Matrix4::Translation(-transform.position);

    auto proj = Matrix4::Perspective(camera.verticalFov, camera.aspectRatio, camera.near, camera.far);

    // Draw
    auto& ctx = *m_Context;
    ctx.Begin();

    RenderShadows(scene);

    ctx.BeginRenderPass(m_Device->AcquireFramebuffer());

    ctx.Bind(m_DefaultPipeline);

    // Draw entities
    scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
    {
        for (auto idx : instance.meshes)
        {
            auto& mesh = scene.meshes[idx];
            auto& material = scene.materials[mesh.materialIdx];

            // Bind globals
            ctx.Uniform("u_Model"_id, local.model);
            ctx.Uniform("u_View"_id, view);
            ctx.Uniform("u_Proj"_id, proj);
            ctx.Uniform("u_CameraPos"_id, transform.position);

            // Bind global light
            ctx.Uniform("u_LightViewProj"_id, m_ShadowViewProj);
            ctx.Uniform("u_LightViewProj1"_id, m_ShadowViewProj1);
            ctx.Uniform("u_LightDir"_id, m_ShadowDir);
            ctx.Uniform("u_LightPlane"_id, m_ShadowPlane);
            ctx.Uniform("u_LightScale"_id, m_ShadowScale);
            ctx.Uniform("u_LightOffset"_id, m_ShadowOffset);

            ctx.Bind("u_EnvIrradiance"_id, scene.environment.irradianceMap);
            ctx.Bind("u_EnvSpecular"_id, scene.environment.specularMap);
            ctx.Bind("u_BRDF"_id, scene.environment.BRDF);
            ctx.Bind("u_ShadowMap"_id, m_MomentMap);

            ctx.Bind("u_BaseColor"_id, material.baseColorMap);
            ctx.Bind("u_MetalRoughness"_id, material.metalRough);
            ctx.Bind("u_Normal"_id, material.normalMap);
            ctx.Bind("u_AO"_id, material.aoMap);
            ctx.Bind("u_Emissive"_id, material.emissive);

            ctx.Uniform("u_BaseColorFactor"_id, material.baseColorFactor);
            ctx.Uniform("u_MetallicFactor"_id, material.metallicFactor);
            ctx.Uniform("u_RoughnessFactor"_id, material.roughnessFactor);

            ctx.Bind(mesh.vertexBuffer);
            ctx.Bind(mesh.indexBuffer);
            ctx.Draw(mesh.numIndices);
        }
    });

    // Draw skybox
    ctx.Bind(m_SkyboxPipeline);

    ctx.Uniform("u_Model"_id, Matrix4::Identity());
    ctx.Uniform("u_View"_id, view);
    ctx.Uniform("u_Proj"_id, proj);

    ctx.Bind("u_EnvIrradiance"_id, scene.environment.irradianceMap);
    ctx.Bind("u_EnvSpecular"_id, scene.environment.specularMap);
    ctx.Bind("u_BRDF"_id, scene.environment.BRDF);
    ctx.Bind("u_ShadowMap"_id, m_MomentMap);

    ctx.Bind("u_Environment"_id, scene.environment.cubeMap);

    ctx.Bind(m_Device->m_Cube.indices);
    ctx.Bind(m_Device->m_Cube.vertices);
    ctx.Draw(m_Device->m_Cube.numIndices);

    // Draw console
    m_DebugConsole->Render(ctx);

    ctx.EndRenderPass();
    ctx.End();

    m_Device->Submit(&ctx);
    m_Device->Present();
}

void SceneRenderer::RenderShadows(Scene& scene)
{
    auto& ctx = *m_Context;

    CalculateCascades(scene);
    auto& cascades = scene.mainDirectionalLight.Get<DirectionalLight>().cascades;

    // Render only depth to the moment MS depth textures
    for (int i = 0; i < kShadowTargets; ++i)
    {
        auto depthImage = m_MomentDepthTextures[i];
        ctx.DepthImageToAttachment(depthImage);

        ctx.BeginRenderPass(m_MomentDepthTargets[i]);
        auto& cascade = cascades[i];
        ctx.Bind(m_MomentDepthOnly);
        scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
        {
            for (auto idx : instance.meshes)
            {
                auto& mesh = scene.meshes[idx];
                auto mvp = cascade.proj * local.model;
                ctx.Uniform("u_MVP"_id, mvp);

                ctx.Bind(mesh.vertexBuffer);
                ctx.Bind(mesh.indexBuffer);
                ctx.Draw(mesh.numIndices);
            }
        });
        ctx.EndRenderPass();
    }

    // Calculate moments from depth values using custom resolve
    ctx.ImageToAttachment(m_MomentMap);
    for (int i = 0; i < kShadowTargets; ++i)
    {
        auto depthImage = m_MomentDepthTextures[i];
        auto target = m_MomentMapTargets[i];

        ctx.DepthAttachmentToImage(depthImage);

        ctx.BeginRenderPass(target);

        ctx.Bind(m_MomentResolveDepth);
        ctx.Bind("u_Depth"_id, depthImage);

        ctx.Bind(m_Device->m_Quad.vertices);
        ctx.Bind(m_Device->m_Quad.indices);
        ctx.Draw(m_Device->m_Quad.numIndices);

        ctx.EndRenderPass();
    }
    ctx.AttachmentToImage(m_MomentMap);

    // TODO: Blur moment map with gaussian kernel
    // TODO: Generate MIP chain for moment map
}

void SceneRenderer::CalculateCascades(Scene& scene)
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

    for (auto& cascade : light.cascades)
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

        for (auto dist : { cascade.start, cascade.end })
        {
            for (auto x : { -view.aspectRatio, view.aspectRatio })
            {
                for (auto y : { -1.0f, 1.0f })
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
        auto camNormal = Vector3(camToWorld.c3);
        auto camPos = Vector3(camToWorld.c4);
        camPos += (view.near + cascade.start) * camNormal;

        auto camD = camNormal.Dot(camPos);
        cascade.frontPlane = Vector4(camNormal, -camD);
    }

    // Set up variables bound during main colour pass
    m_ShadowViewProj = light.cascades[0].proj;
    m_ShadowViewProj1 = light.cascades[1].proj;
    m_ShadowDir = -Vector3(Matrix4(lightOrigin.rotation).c3);
    for (int i = 1; i < DirectionalLight::kNumCascades; ++i)
    {
        auto& cascade = light.cascades[i];
        // Multiply to transition from 0->1 in cascade overlap regions
        float factor = 1.0f / (light.cascades[i - 1].end - cascade.start);
        m_ShadowPlane[i - 1] = factor * cascade.frontPlane;
        m_ShadowScale[i - 1] = Vector4(cascade.scale);
        m_ShadowOffset[i - 1] = Vector4(cascade.offset);
    }

    // TODO: Adjust cascade index so that all cascades in a quad use the same cascade?
}

}
