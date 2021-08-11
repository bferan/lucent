#include <fstream>

#include "GLFW/glfw3.h"

#include "device/Device.hpp"
#include "rendering/SceneRenderer.hpp"
#include "scene/Importer.hpp"
#include "scene/HdrImporter.hpp"

namespace lucent::demos
{

class BasicDemo
{
public:
    void Init()
    {
        LC_INFO("Welcome to LUCENT!");
        m_Renderer = std::make_unique<SceneRenderer>(&m_Device);

        Importer importer(&m_Device);

        //importer.Import(m_Scene, "models/Plane.glb");
        importer.Import(m_Scene, "models/DamagedHelmet.glb");

        HdrImporter hdrImporter(&m_Device);
        m_Scene.environment = hdrImporter.Import("textures/shanghai_bund_4k.hdr");

        // Create camera entity
        m_Player = m_Scene.Create();
        m_Player.Assign(Transform{ .position = { 0.0f, 2.0f, 2.0f }});
        m_Player.Assign(Camera{ .horizontalFov = kHalfPi, .aspectRatio = 1600.0f / 900.0f });
    }

    void Draw(float dt)
    {
        static float timer = 0.0f;
        timer += dt;

        auto& input = m_Device.m_Input->GetState();
        auto& transform = m_Player.Get<Transform>();
        auto& camera = m_Player.Get<Camera>();

        if (!m_Renderer->m_DebugConsole->Active())
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
        }

        // Update console
        m_Renderer->m_DebugConsole->Update(input, dt);

        m_Renderer->Render(m_Scene);

        m_Device.m_Input->Reset();
    }

public:
    LogStdOut logStdOut{};
    Device m_Device{};
    std::unique_ptr<SceneRenderer> m_Renderer{};

    Scene m_Scene{};
    Entity m_Player;
};

}

int main()
{
    lucent::demos::BasicDemo demo;
    demo.Init();

    double prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(demo.m_Device.m_Window))
    {
        glfwPollEvents();
        double time = glfwGetTime();
        demo.Draw(static_cast<float>(time - prevTime));
        prevTime = time;
    }
    return 0;
}

