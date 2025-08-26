#include "rendering/Engine.hpp"
#include "scene/Importer.hpp"
#include "scene/HdrImporter.hpp"
#include "scene/Transform.hpp"
#include "scene/Camera.hpp"

using namespace lucent;

// Example usage: importing a model and setting up a basic light and camera
void InitScene(Engine& engine, Scene& scene)
{
    auto modelPath = "data/models/bust/marble_bust_01_4k.gltf";
    auto hdrPath = "data/textures/old_hall_4k.hdr";

    Importer importer(engine.GetDevice());

    // Import entity for model
    auto model = importer.Import(scene, modelPath);
    model.SetScale(5.0f);
	model.SetRotation(Quaternion::AxisAngle(Vector3::Up(), kHalfPi));

    HdrImporter hdrImporter(engine.GetDevice());
    scene.environment = hdrImporter.Import(hdrPath);

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

