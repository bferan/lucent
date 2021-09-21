#include "SceneRenderer.hpp"

#include "core/Utility.hpp"
#include "device/Context.hpp"
#include "rendering/Geometry.hpp"

#include <random>

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

SceneRenderer::SceneRenderer(const RenderSettings& settings, Device* device)
    : m_Settings(settings), m_Device(device)
{
    m_Context = m_Device->CreateContext();

    // Output
    m_SceneColor = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kRGBA8,
        .addressMode = TextureAddressMode::kClampToBorder
    });

    m_GDepth = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kDepth32F,
        .addressMode = TextureAddressMode::kClampToEdge,
        .filter = TextureFilter::kNearest,
        .usage = TextureUsage::kDepthAttachment
    });

    m_SceneColorFramebuffer = m_Device->CreateFramebuffer(FramebufferSettings{
        .colorTextures = { m_SceneColor },
        .depthTexture = m_GDepth
    });

    m_DefaultPipeline = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "Standard.shader", .framebuffer = m_SceneColorFramebuffer });
    m_SkyboxPipeline = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "Skybox.shader", .framebuffer = m_SceneColorFramebuffer });

    // Create debug console (temp)
    m_DebugConsole = std::make_unique<DebugConsole>(m_Device, m_SceneColorFramebuffer);
    m_DebugShapeBuffer = m_Device->CreateBuffer(BufferType::kStorage, sizeof(DebugShapeBuffer));
    m_DebugShapeBuffer->Clear(sizeof(DebugShapeBuffer), 0);
    m_DebugShapes = static_cast<DebugShapeBuffer*>(m_DebugShapeBuffer->Map());
    m_DebugShapePipeline = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "DebugShape.shader",
        .framebuffer = m_SceneColorFramebuffer
    });

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
        .width = DirectionalLight::kMapWidth,
        .height = DirectionalLight::kMapWidth,
        .format = TextureFormat::kDepth16U,
        .usage = TextureUsage::kDepthAttachment
    });

    for (int i = 0; i < kShadowTargets; ++i)
    {
        auto depth = m_MomentDepthTextures.emplace_back(m_Device->CreateTexture(TextureSettings{
            .width = DirectionalLight::kMapWidth,
            .height = DirectionalLight::kMapWidth,
            .samples = 8,
            .format = TextureFormat::kDepth16U,
            .usage = TextureUsage::kDepthAttachment
        }));

        m_MomentDepthTargets.emplace_back(m_Device->CreateFramebuffer(FramebufferSettings{
            .depthTexture = depth
        }));

        m_MomentMapTargets.emplace_back(m_Device->CreateFramebuffer(FramebufferSettings{
            .colorTextures = { m_MomentMap },
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

    // GBuffer
    auto levels = (uint32)Floor(Log2((float)Max(m_Settings.width, m_Settings.height))) + 1;

    m_GBaseColor = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kRGBA32F
    });

    m_GMinZ = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .levels = levels,
        .format = TextureFormat::kR32F,
        .addressMode = TextureAddressMode::kClampToEdge,
        .filter = TextureFilter::kNearest,
        .usage = TextureUsage::kReadWrite
    });

    m_GNormals = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kRGBA32F
    });

    m_GMetalRoughness = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kRG32F
    });

    m_GFramebuffer = m_Device->CreateFramebuffer(FramebufferSettings{
        .colorTextures = { m_GBaseColor, m_GNormals, m_GMetalRoughness },
        .depthTexture = m_GDepth,
    });

    m_GenGBuffer = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "GenGBuffer.shader",
        .framebuffer = m_GFramebuffer
    });

    m_GenMinZ = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "MinZ.shader",
        .type = PipelineType::kCompute
    });

    m_TransferBuffer = m_Device->CreateBuffer(BufferType::kStaging, 32 * 1024 * 1024);

    // AO
    m_AOMap = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kR32F,
        .usage = TextureUsage::kReadWrite
    });

    m_ComputeAO = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "GTAO.shader",
        .type = PipelineType::kCompute
    });

    // SSR
    m_BlueNoise = GenerateHaltonNoiseTexture(128);

    m_ReflectionResult = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kRGBA32F,
        .usage = TextureUsage::kReadWrite
    });

    m_ReflectionResolve = m_Device->CreateTexture(TextureSettings{
        .width = m_Settings.width,
        .height = m_Settings.height,
        .format = TextureFormat::kRGBA32F,
        .usage = TextureUsage::kReadWrite
    });

    m_TraceReflections = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "TraceMinZ.shader",
        .type = PipelineType::kCompute
    });

    m_ResolveReflections = m_Device->CreatePipeline(PipelineSettings{
        .shaderName = "ResolveSSR.shader",
        .type = PipelineType::kCompute
    });
}

void SceneRenderer::Render(Scene& scene)
{
    // Clear debug info
    *m_DebugShapes = {};
    m_DebugShapeBuffer->Flush(VK_WHOLE_SIZE, 0);

    // Find first camera
    auto& transform = scene.mainCamera.Get<Transform>();
    auto& camera = scene.mainCamera.Get<Camera>();

    // Calc camera position
    auto view = Matrix4::RotationX(kPi) *
        Matrix4::RotationX(-camera.pitch) *
        Matrix4::RotationY(-camera.yaw) *
        Matrix4::Translation(-transform.position);

    auto invView = Matrix4::Translation(transform.position) *
        Matrix4::RotationY(camera.yaw) *
        Matrix4::RotationX(camera.pitch) *
        Matrix4::RotationX(kPi);

    auto proj = Matrix4::Perspective(camera.verticalFov, camera.aspectRatio, camera.near, camera.far);

    auto& ctx = *m_Context;
    ctx.Begin();

    RenderShadows(scene);
    RenderGBuffer(scene, view, proj);
    RenderAmbientOcclusion(scene, view, proj, invView);
    RenderReflections(scene, proj, invView);

    // Main forward pass
    ctx.BeginRenderPass(m_SceneColorFramebuffer);
    ctx.Clear();

    ctx.BindPipeline(m_DefaultPipeline);

    // Draw entities
    scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
    {
        for (auto idx: instance.meshes)
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

            ctx.BindTexture("u_EnvIrradiance"_id, scene.environment.irradianceMap);
            ctx.BindTexture("u_EnvSpecular"_id, scene.environment.specularMap);
            ctx.BindTexture("u_BRDF"_id, scene.environment.BRDF);
            ctx.BindTexture("u_ShadowMap"_id, m_MomentMap);
            ctx.BindTexture("u_AOMap"_id, m_AOMap);
            ctx.BindTexture("u_NormalMap"_id, m_GNormals);
            ctx.BindTexture("u_Reflections"_id, m_ReflectionResolve);

            ctx.BindTexture("u_BaseColor"_id, material.baseColorMap);
            ctx.BindTexture("u_MetalRoughness"_id, material.metalRough);
            ctx.BindTexture("u_Normal"_id, material.normalMap);
            ctx.BindTexture("u_AO"_id, material.aoMap);
            ctx.BindTexture("u_Emissive"_id, material.emissive);

            ctx.Uniform("u_BaseColorFactor"_id, material.baseColorFactor);
            ctx.Uniform("u_MetallicFactor"_id, material.metallicFactor);
            ctx.Uniform("u_RoughnessFactor"_id, material.roughnessFactor);

            ctx.BindBuffer(mesh.vertexBuffer);
            ctx.BindBuffer(mesh.indexBuffer);
            ctx.Draw(mesh.numIndices);
        }
    });

    // Draw skybox
    ctx.BindPipeline(m_SkyboxPipeline);

    ctx.Uniform("u_Model"_id, Matrix4::Identity());
    ctx.Uniform("u_View"_id, view);
    ctx.Uniform("u_Proj"_id, proj);

    ctx.BindTexture("u_EnvIrradiance"_id, scene.environment.irradianceMap);
    ctx.BindTexture("u_EnvSpecular"_id, scene.environment.specularMap);
    ctx.BindTexture("u_BRDF"_id, scene.environment.BRDF);
    ctx.BindTexture("u_ShadowMap"_id, m_MomentMap);
    ctx.BindTexture("u_AOMap"_id, m_AOMap);

    ctx.BindTexture("u_Environment"_id, scene.environment.cubeMap);

    ctx.BindBuffer(g_Cube.indices);
    ctx.BindBuffer(g_Cube.vertices);
    ctx.Draw(g_Cube.numIndices);

    ctx.EndRenderPass();
    ctx.End();

    m_Device->Submit(&ctx, false);

    /* ----- Debug rendering: ----- */
    m_DebugShapeBuffer->Invalidate(VK_WHOLE_SIZE, 0);

    auto outputFramebuffer = m_Device->AcquireFramebuffer();
    ctx.Begin();

    ctx.BeginRenderPass(m_SceneColorFramebuffer);
    RenderDebugOverlay(scene, view, proj);
    ctx.EndRenderPass();

    // Blit image to output framebuffer
    auto srcTexture = m_SceneColor;
    auto dstTexture = outputFramebuffer->colorTextures.front();

    ctx.BlitTexture(srcTexture, 0, 0, dstTexture, 0, 0, dstTexture->width, dstTexture->height);
    ctx.End();

    m_Device->Submit(&ctx, true);

    m_Device->Present();
}

void SceneRenderer::RenderGBuffer(Scene& scene, Matrix4 view, Matrix4 proj) const
{
    auto& ctx = *m_Context;

    ctx.BeginRenderPass(m_GFramebuffer);
    ctx.Clear();

    ctx.BindPipeline(m_GenGBuffer);
    scene.Each<MeshInstance, Transform>([&](MeshInstance& instance, Transform& local)
    {
        for (auto idx: instance.meshes)
        {
            auto& mesh = scene.meshes[idx];
            auto& material = scene.materials[mesh.materialIdx];

            auto mv = view * local.model;
            auto mvp = proj * mv;

            // Bind globals
            ctx.Uniform("u_MVP"_id, mvp);
            ctx.Uniform("u_MV"_id, mv);

            // Bind material data
            ctx.BindTexture("u_BaseColor"_id, material.baseColorMap);
            ctx.BindTexture("u_MetalRoughness"_id, material.metalRough);
            ctx.BindTexture("u_Normal"_id, material.normalMap);

            ctx.Uniform("u_BaseColorFactor"_id, material.baseColorFactor);
            ctx.Uniform("u_MetallicFactor"_id, material.metallicFactor);
            ctx.Uniform("u_RoughnessFactor"_id, material.roughnessFactor);

            ctx.BindBuffer(mesh.vertexBuffer);
            ctx.BindBuffer(mesh.indexBuffer);
            ctx.Draw(mesh.numIndices);
        }
    });
    ctx.EndRenderPass();

    // Create min-Z depth pyramid
    {
        ctx.CopyTexture(m_GDepth, 0, 0, m_TransferBuffer, 0, m_GDepth->width, m_GDepth->height);
        ctx.CopyTexture(m_TransferBuffer, 0, m_GMinZ, 0, 0, m_GMinZ->width, m_GMinZ->height);

        uint32 width = m_GMinZ->width;
        uint32 height = m_GMinZ->height;

        ctx.BindPipeline(m_GenMinZ);
        for (int level = 1; level < m_GMinZ->levels; ++level)
        {
            width = Max(width / 2u, 1u);
            height = Max(height / 2u, 1u);

            ctx.BindTexture("u_Input"_id, m_GMinZ, level - 1);
            ctx.BindImage("u_Output"_id, m_GMinZ, level);

            ctx.Dispatch(width, height, 1);
        }
    }

}

void SceneRenderer::RenderShadows(Scene& scene)
{
    auto& ctx = *m_Context;

    CalculateCascades(scene);
    auto& cascades = scene.mainDirectionalLight.Get<DirectionalLight>().cascades;

    // Render depth to the moment MS depth textures
    for (int i = 0; i < kShadowTargets; ++i)
    {
        auto depthImage = m_MomentDepthTextures[i];

        ctx.BeginRenderPass(m_MomentDepthTargets[i]);
        ctx.Clear();

        auto& cascade = cascades[i];
        ctx.BindPipeline(m_MomentDepthOnly);
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

    // Calculate moments from depth values using custom resolve
    for (int i = 0; i < kShadowTargets; ++i)
    {
        auto depthImage = m_MomentDepthTextures[i];
        auto target = m_MomentMapTargets[i];

        ctx.BeginRenderPass(target);

        ctx.BindPipeline(m_MomentResolveDepth);
        ctx.BindTexture("u_Depth"_id, depthImage);

        ctx.BindBuffer(g_Quad.vertices);
        ctx.BindBuffer(g_Quad.indices);
        ctx.Draw(g_Quad.numIndices);

        ctx.EndRenderPass();
    }

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

void SceneRenderer::RenderAmbientOcclusion(Scene& scene, Matrix4 view, Matrix4 proj, Matrix4 invView) const
{
    auto& ctx = *m_Context;

    // Compute GTAO
    ctx.BindPipeline(m_ComputeAO);
    ctx.BindTexture("u_Depth"_id, m_GMinZ, 0);
    ctx.BindTexture("u_Normals"_id, m_GNormals);
    ctx.BindImage("u_AO"_id, m_AOMap);

    float w = (float)m_Settings.width;
    float h = (float)m_Settings.height;

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
    auto& input = m_Device->m_Input->GetState();

    struct MousePos
    {
        uint32 x;
        uint32 y;
    };
    auto mousePos = MousePos{ (uint32)Round(input.cursorPos.x), (uint32)Round(input.cursorPos.y) };

    ctx.BindBuffer("DebugShapes"_id, m_DebugShapeBuffer);
    ctx.Uniform("u_DebugCursorPos"_id, mousePos);
    ctx.Uniform("u_ViewToWorld"_id, invView);

    /**********************/
    ctx.Dispatch(m_Settings.width, m_Settings.height, 1);

    // Spatial blur
    // Temporal blur
}

void SceneRenderer::RenderDebugOverlay(Scene& scene, Matrix4 view, Matrix4 proj) const
{
    auto& ctx = *m_Context;

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

    m_DebugConsole->Render(ctx);
}

Texture* SceneRenderer::GenerateHaltonNoiseTexture(uint32 size)
{
    auto data = GenerateBlueNoise(size);

//    uint32 numSamples = size * size;
//    std::vector<Vector2> data(numSamples);

//    constexpr int dims = 2;
//    constexpr int base[] = { 2, 3 };
//
//    int num[] = { 0, 0 };
//    int denom[] = { 1, 1 };
//
//    for (int i = 0; i < numSamples; ++i)
//    {
//        for (int x = 0; x < dims; ++x)
//        {
//            int diff = denom[x] - num[x];
//            if (diff == 1)
//            {
//                num[x] = 1;
//                denom[x] *= base[x];
//            }
//            else
//            {
//                int div = denom[x] / base[x];
//                while (diff <= div)
//                    div /= base[x];
//                num[x] = (base[x] + 1) * div - diff;
//            }
//        }
//
//        data[i] = {
//            (float)num[0] / (float)denom[0],
//            (float)num[1] / (float)denom[1],
//        };
//    }

    auto tex = m_Device->CreateTexture(TextureSettings{
        .width = size,
        .height = size,
        .format = TextureFormat::kRG32F,
        .addressMode = TextureAddressMode::kRepeat,
        .filter = TextureFilter::kNearest
    });
    tex->Upload(data.size() * sizeof(Vector2), data.data());

    return tex;
}

void SceneRenderer::RenderReflections(Scene& scene, Matrix4 proj, Matrix4 invView) const
{
    auto& ctx = *m_Context;

    // Trace reflection rays
    ctx.BindPipeline(m_TraceReflections);
    ctx.BindTexture("u_MinZ"_id, m_GMinZ);
    ctx.BindTexture("u_HaltonNoise"_id, m_BlueNoise);
    ctx.BindTexture("u_Normals"_id, m_GNormals);
    ctx.BindTexture("u_MetalRoughness"_id, m_GMetalRoughness);
    ctx.BindImage("u_Result"_id, m_ReflectionResult);

    ctx.Uniform("u_ScreenToView"_id, Vector4(
        2.0f * (1.0f / proj(0, 0)),
        2.0f * (1.0f / proj(1, 1)),
        proj(2, 2), proj(2, 3)));

    ctx.Uniform("u_Proj"_id, proj);

    ctx.Dispatch(m_Settings.width, m_Settings.height, 1);

    // Resolve reflections
    ctx.BindPipeline(m_ResolveReflections);
    ctx.BindTexture("u_Rays"_id, m_ReflectionResult);
    ctx.BindTexture("u_PrevColor"_id, m_SceneColor);
    ctx.BindTexture("u_Depth"_id, m_GMinZ);
    ctx.BindTexture("u_BaseColor"_id, m_GBaseColor);
    ctx.BindTexture("u_Normals"_id, m_GNormals);
    ctx.BindTexture("u_MetalRoughness"_id, m_GMetalRoughness);
    ctx.BindImage("u_Result"_id, m_ReflectionResolve);

    ctx.Uniform("u_ScreenToView"_id, Vector4(
        2.0f * (1.0f / proj(0, 0)),
        2.0f * (1.0f / proj(1, 1)),
        proj(2, 2), proj(2, 3)));

    ctx.Uniform("u_Proj"_id, proj);

    ctx.Dispatch(m_Settings.width, m_Settings.height, 1);
}

}
