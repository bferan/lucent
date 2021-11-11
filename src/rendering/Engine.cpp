#include "Engine.hpp"

#include "GLFW/glfw3.h"

#include "device/vulkan/VulkanDevice.hpp"

#include "features/GeometryPass.hpp"
#include "features/LightingPass.hpp"
#include "features/ScreenSpaceReflectionsPass.hpp"
#include "features/AmbientOcclusionPass.hpp"
#include "features/MomentShadowPass.hpp"
#include "features/PostProcessPass.hpp"
#include "features/DebugOverlayPass.hpp"
#include "rendering/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "scene/Transform.hpp"

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

std::unique_ptr<Engine> Engine::s_Engine;

Engine* Engine::Init(RenderSettings settings)
{
    if (!s_Engine)
    {
        s_Engine = std::unique_ptr<Engine>(new Engine(std::move(settings)));
    }
    return s_Engine.get();
}

Engine* Engine::Instance()
{
    LC_ASSERT(s_Engine && "requires initialization!");
    return s_Engine.get();
}

// TODO: Remove or extract to separate file
// Ideally user provides existing window to the engine, might have to change debug functions in this case
class Window
{
public:
    GLFWwindow* window;
};

Engine::Engine(RenderSettings settings)
{
    // Set up GLFW
    if (!glfwInit())
        return;

    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = std::make_unique<Window>(Window{
        glfwCreateWindow((int)settings.viewportWidth, (int)settings.viewportHeight, "Lucent", nullptr, nullptr)
    });
    LC_ASSERT(m_Window->window != nullptr);

    m_Device = std::make_unique<VulkanDevice>(m_Window->window);
    m_Input = std::make_unique<Input>(m_Window->window);
    m_Console = std::make_unique<DebugConsole>(*this, 120);

    settings.InitializeDefaultResources(m_Device.get());
    m_SceneRenderer = std::make_unique<Renderer>(m_Device.get(), std::move(settings));
    m_BuildSceneRenderer = BuildDefaultSceneRenderer;

    m_BuildSceneRenderer(this, *m_SceneRenderer);
}

bool Engine::Update()
{
    if (glfwWindowShouldClose(m_Window->window))
        return false;

    glfwPollEvents();
    double time = glfwGetTime();
    auto dt = float(time - m_LastUpdateTime);

    UpdateDebug(dt);
    if (!m_SceneRenderer->Render(*m_ActiveScene))
    {
        LC_INFO("Rebuilding scene renderer");

        m_Device->WaitIdle();
        m_Device->RebuildSwapchain();

        int width, height;
        glfwGetFramebufferSize(m_Window->window, &width, &height);

        auto& settings = m_SceneRenderer->GetSettings();
        settings.viewportWidth = width;
        settings.viewportHeight = height;

        m_ActiveScene->mainCamera.Get<Camera>().aspectRatio = (float)width / (float)height;

        m_SceneRenderer->Clear();
        m_BuildSceneRenderer(this, *m_SceneRenderer);
    };
    m_Input->Reset();

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
    auto& input = m_Input->GetState();

    if (!m_Console->Active())
    {
        m_ActiveScene->Each<Transform, Camera>([&](auto& transform, auto& camera)
        {
            // TODO: Remove temporary camera movement controls to debug camera

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

            constexpr float kFlySpeedMultiplier = 3.0f;
            float multiplier = input.KeyDown(LC_KEY_LEFT_CONTROL) ? kFlySpeedMultiplier : 1.0f;

            const float speed = 5.0f;
            transform.position += dt * speed * multiplier * velocityWorld;
        });
    }
    // Update debug console
    m_Console->Update(input, dt);
}

DebugConsole* Engine::GetConsole()
{
    return m_Console.get();
}

Input& Engine::GetInput()
{
    return *m_Input;
}

const RenderSettings& Engine::GetRenderSettings()
{
    return m_SceneRenderer->GetSettings();
}

}
