#include "Engine.hpp"

#include "GLFW/glfw3.h"

#include "device/vulkan/VulkanDevice.hpp"

#include "rendering/GeometryPass.hpp"
#include "rendering/LightingPass.hpp"
#include "rendering/ScreenSpaceReflectionsPass.hpp"
#include "rendering/AmbientOcclusionPass.hpp"
#include "rendering/MomentShadowPass.hpp"
#include "rendering/PostProcessPass.hpp"
#include "rendering/DebugOverlayPass.hpp"

namespace lucent
{

static void BuildDefaultSceneRenderer(Engine* engine, Renderer& renderer)
{
    auto sceneRadiance = CreateSceneRadianceTarget(renderer);

    auto gBuffer = AddGeometryPass(renderer);
    auto hiZ = AddGenerateHiZPass(renderer, gBuffer.depth);
    auto shadowMoments = AddMomentShadowPass(renderer);
    auto gtao = AddGTAOPass(renderer, gBuffer, hiZ);
    auto ssr = AddScreenSpaceReflectionsPass(renderer, gBuffer, hiZ, sceneRadiance);

    AddLightingPass(renderer, gBuffer, hiZ, sceneRadiance, shadowMoments, gtao, ssr);
    auto output = AddPostProcessPass(renderer, sceneRadiance);

    AddDebugOverlayPass(renderer, engine->GetConsole(), output);

    renderer.AddPresentPass(output);
}

Engine::Engine()
{
    // Set up GLFW
    if (!glfwInit())
        return;

    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(
        1600, 900,
        "Lucent",
        nullptr, nullptr);

    LC_ASSERT(m_Window != nullptr);

    m_Device = std::make_unique<VulkanDevice>(m_Window);
    m_Console = std::make_unique<DebugConsole>(m_Device.get(), 120);

    RenderSettings settings{ .viewportWidth = 1600, .viewportHeight = 900 };
    m_SceneRenderer = std::make_unique<Renderer>(m_Device.get(), settings);
    m_BuildSceneRenderer = BuildDefaultSceneRenderer;

    m_BuildSceneRenderer(this, *m_SceneRenderer);
}

bool Engine::Update()
{
    if (glfwWindowShouldClose(m_Window))
        return false;

    glfwPollEvents();
    double time = glfwGetTime();
    float dt = time - m_LastUpdateTime;

    UpdateDebug(dt);
    if (!m_SceneRenderer->Render(*m_ActiveScene))
    {
        LC_INFO("Rebuilding scene renderer");

        m_Device->WaitIdle();
        m_Device->RebuildSwapchain();

        int width, height;
        glfwGetFramebufferSize(m_Window, &width, &height);

        auto settings = m_SceneRenderer->GetSettings();
        settings.viewportWidth = width;
        settings.viewportHeight = height;

        m_ActiveScene->mainCamera.Get<Camera>().aspectRatio = (float)width / (float)height;

        m_SceneRenderer->Clear();
        m_SceneRenderer->SetSettings(settings);
        m_BuildSceneRenderer(this, *m_SceneRenderer);
    };
    m_Device->m_Input->Reset();

    m_LastUpdateTime = time;
    return true;
}

Device* Engine::GetDevice()
{
    return m_Device.get();
}

Scene* Engine::CreateScene()
{
    auto scene = m_Scenes.emplace_back(std::make_unique<Scene>()).get();
    m_ActiveScene = scene;
    return scene;
}

void Engine::UpdateDebug(float dt)
{
    auto& input = m_Device->m_Input->GetState();

    if (!m_Console->Active())
    {
        m_ActiveScene->Each<Transform, Camera>([&](auto& transform, auto& camera)
        {
            // Update camera pos
            const float hSensitivity = 0.8f;
            const float vSensitivity = 1.0f;
            camera.yaw += dt * hSensitivity * -input.cursorDelta.x;
            camera.pitch += dt * vSensitivity * -input.cursorDelta.y;
            camera.pitch = Clamp(camera.pitch, -kHalfPi, kHalfPi);

            auto rotation = Matrix4::RotationY(camera.yaw);

            Vector3 velocity;
            if (input.KeyDown(LC_KEY_W)) velocity += Vector3::Forward();
            if (input.KeyDown(LC_KEY_S)) velocity += Vector3::Back();
            if (input.KeyDown(LC_KEY_A)) velocity += Vector3::Left();
            if (input.KeyDown(LC_KEY_D)) velocity += Vector3::Right();

            velocity.Normalize();
            auto velocityWorld = Vector3(rotation * Vector4(velocity));

            if (input.KeyDown(LC_KEY_SPACE)) velocityWorld += Vector3::Up();
            if (input.KeyDown(LC_KEY_LEFT_SHIFT)) velocityWorld += Vector3::Down();

            float multiplier = input.KeyDown(LC_KEY_LEFT_CONTROL) ? 3.0f : 1.0f;

            const float speed = 5.0f;
            transform.position += dt * speed * multiplier * velocityWorld;
        });
    }

    if (input.KeyPressed(LC_KEY_R))
        m_Device->ReloadPipelines();

    // Update console
    m_Console->Update(input, dt);
}

DebugConsole* Engine::GetConsole()
{
    return m_Console.get();
}

}
