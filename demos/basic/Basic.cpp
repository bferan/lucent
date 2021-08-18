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

        auto helmet = importer.Import(m_Scene, "models/DamagedHelmet.glb");
        helmet.Get<Transform>().position += Vector3::Up();
        ApplyTransform(helmet);

        auto box = importer.Import(m_Scene, "models/BoxTextured.glb");
        box.Get<Transform>().position = Vector3(2.0f, 0.0, -1.0f);
        ApplyTransform(box);

        m_Rotate = helmet;

        auto plane = importer.Import(m_Scene, "models/Plane.glb");
        auto& planeTransform = plane.Get<Transform>();
        planeTransform.position += Vector3::Down();
        planeTransform.scale = 10.0f;
        ApplyTransform(plane);

        HdrImporter hdrImporter(&m_Device);
        m_Scene.environment = hdrImporter.Import("textures/chinese_garden_4k.hdr");

        // Create camera entity
        m_Scene.mainCamera = m_Scene.CreateEntity();
        m_Scene.mainCamera.Assign(Camera{ .horizontalFov = kHalfPi, .aspectRatio = 1600.0f / 900.0f });
        m_Scene.mainCamera.Assign(Transform{ .position = { 0.0f, 2.0f, 2.0f }});

        auto lightPosition = Vector3{ 3.0f, 4.0f, -4.1f };
        auto lightRotation = Matrix4::Rotation(Matrix4::LookAt(lightPosition, Vector3::Zero()));

        m_Scene.mainDirectionalLight = m_Scene.CreateEntity();
        m_Scene.mainDirectionalLight.Assign(DirectionalLight{});
        m_Scene.mainDirectionalLight.Assign(Transform{
            .rotation = lightRotation,
            .position = lightPosition
        });
    }

    void Draw(float dt)
    {
        auto& input = m_Device.m_Input->GetState();

        float rotateSpeed = 0.5f;
        auto rotate = Quaternion::AxisAngle(Vector3::Up(), rotateSpeed * dt);

        auto& rot = m_Rotate.Get<Transform>().rotation;
        rot = rotate * rot;
        ApplyTransform(m_Rotate);

        if (!m_Renderer->m_DebugConsole->Active())
        {
            m_Scene.Each<Transform, Camera>([&](auto& transform, auto& camera)
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

        // Update console
        m_Renderer->m_DebugConsole->Update(input, dt);

        m_Renderer->Render(m_Scene);

        m_Device.m_Input->Reset();
    }

public:
    LogStdOut logStdOut{};
    Device m_Device{};
    std::unique_ptr<SceneRenderer> m_Renderer{};

    Entity m_Rotate;

    Scene m_Scene{};
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

