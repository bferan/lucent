#include "rendering/Engine.hpp"
#include "scene/Importer.hpp"
#include "scene/HdrImporter.hpp"
#include "scene/Transform.hpp"
#include "scene/Camera.hpp"

using namespace lucent;

// Example usage: importing a model and setting up a basic light and camera
void InitScene(Engine& engine, Scene& scene)
{
    auto robotLocation = "models/robo/scene.gltf";
    auto hdrLocation = "textures/rooitou_park_4k.hdr";

    Importer importer(engine.GetDevice());

    // Import robot entity from model
    auto robot = importer.Import(scene, robotLocation);
    robot.SetScale(0.2f);

    HdrImporter hdrImporter(engine.GetDevice());
    scene.environment = hdrImporter.Import(hdrLocation);

    // Create camera entity
    scene.mainCamera = scene.CreateEntity();
    scene.mainCamera.Assign(Camera{ .verticalFov = kHalfPi * 0.7, .aspectRatio = 1600.0f / 900.0f });
    scene.mainCamera.Assign(Transform{ .position = { 0.0f, 2.0f, 2.0f }});

    // Create directional light
    auto lightPos = Vector3(-0.3, 1.0f, 0.3f);
    auto light = scene.CreateEntity();

    light.Assign(DirectionalLight{
        .color = Color(201 / 255.0, 226 / 255.0, 255 / 255.0, 1.0),
        .cascades = {
            { .start = 0.0f, .end = 12.0f },
            { .start = 10.0f, .end = 32.0f },
            { .start = 30.0f, .end = 70.0f },
            { .start = 65.0f, .end = 150.0f }
        }
    });
    light.Assign(Transform{ .position = lightPos });
    light.SetRotation(Matrix4::Rotation(Matrix4::LookAt(lightPos, Vector3::Zero())));

    scene.mainDirectionalLight = light;
}

int main()
{
    auto engine = Engine::Init();
    auto scene = engine->CreateScene();

    InitScene(*engine, *scene);
    while (engine->Update());

    return 0;
}

