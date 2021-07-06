#include "Basic.hpp"

#include <iostream>
#include <fstream>

#include "GLFW/glfw3.h"

#include "scene/Importer.hpp"

namespace lucent::demos
{

void BasicDemo::Init()
{
    m_Renderer = std::make_unique<SceneRenderer>(&m_Device);

    Importer importer(&m_Device, m_Renderer->m_Pipeline);

    importer.Import(m_Scene, "models/Plane.glb");

    auto helm = importer.Import(m_Scene, "models/DamagedHelmet.glb");
    m_Scene.transforms[helm].position = { -2.0f, 2.0f, 0.0f };

    auto avo = importer.Import(m_Scene, "models/Avocado.glb");
    m_Scene.transforms[avo].scale = 40.0f;
    m_Scene.transforms[avo].position = { 2.0f, 2.0f, 0.0f };

    // Create camera entity
    m_Player = m_Scene.entities.Create();
    m_Scene.transforms.Assign(m_Player, Transform{.position = {0.0f, 2.0f, 2.0f }});
    m_Scene.cameras.Assign(m_Player, Camera{ .horizontalFov = kHalfPi, .aspectRatio = 1600.0f / 900.0f });
}

void BasicDemo::Draw(float dt)
{
    static float timer = 0.0f;
    timer += dt;

    // Update camera pos
    auto& input = m_Device.m_Input->GetState();
    auto& transform = m_Scene.transforms[m_Player];
    auto& camera = m_Scene.cameras[m_Player];

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

    if (input.KeyDown(LC_KEY_ESCAPE)) exit(0);

    float multiplier = input.KeyDown(LC_KEY_LEFT_CONTROL) ? 3.0f : 1.0f;

    const float speed = 5.0f;
    transform.position += dt * speed * multiplier * velocityWorld;

    m_Renderer->Render(m_Scene);

    m_Device.m_Input->Reset();
}

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
