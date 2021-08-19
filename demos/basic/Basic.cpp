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

        // Axes
        auto boxX = importer.Import(m_Scene, "models/BoxTextured.glb");
        boxX.Get<Transform>().position = Vector3(5.0f, -0.5, 0.0f);
        ApplyTransform(boxX);
        m_Scene.materials[0].baseColorFactor = Color::Red();

        auto boxZ = importer.Import(m_Scene, "models/BoxTextured.glb");
        boxZ.Get<Transform>().position = Vector3(0.0f, -0.5, 5.0f);
        ApplyTransform(boxZ);
        m_Scene.materials[1].baseColorFactor = Color::Blue();

        auto helmet = importer.Import(m_Scene, "models/DamagedHelmet.glb");
        helmet.Get<Transform>().position += Vector3::Up();
        ApplyTransform(helmet);

        auto helmet2 = importer.Import(m_Scene, "models/DamagedHelmet.glb");
        helmet2.Get<Transform>().position = { 35.0f, 1.0f, 35.0f };
        ApplyTransform(helmet2);

        m_Rotate = helmet;

        auto plane = importer.Import(m_Scene, "models/Plane.glb");
        auto& planeTransform = plane.Get<Transform>();
        planeTransform.position += Vector3::Down();
        planeTransform.scale = 10.0f;
        ApplyTransform(plane);

        HdrImporter hdrImporter(&m_Device);
        m_Scene.environment = hdrImporter.Import("textures/shanghai_bund_4k.hdr");

        // Create camera entity
        m_Scene.mainCamera = m_Scene.CreateEntity();
        m_Scene.mainCamera.Assign(Camera{ .verticalFov = kHalfPi, .aspectRatio = 1600.0f / 900.0f });
        m_Scene.mainCamera.Assign(Transform{ .position = { 0.0f, 2.0f, 2.0f }});

        // Create directional light
        auto lightPos = Vector3(3.0f, 4.0f, -4.1f);

        m_Scene.mainDirectionalLight = m_Scene.CreateEntity();

        m_Scene.mainDirectionalLight.Assign(DirectionalLight{
            .cascades = {
                { .start = 0.0f, .end = 10.0f },
                { .start = 13.0f, .end = 45.0f },
                { .start = 40.0f, .end = 80.0f },
                { .start = 75.0f, .end = 150.0f }
            }
        });

        m_Scene.mainDirectionalLight.Assign(Transform{
            .rotation = Matrix4::Rotation(Matrix4::LookAt(lightPos, Vector3::Zero())),
            .position = lightPos
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

