#include "rendering/Engine.hpp"
#include "scene/Importer.hpp"
#include "scene/HdrImporter.hpp"
#include "scene/ModelInstance.hpp"
#include "rendering/PbrMaterial.hpp"
#include "scene/Transform.hpp"
#include "scene/Camera.hpp"

using namespace lucent;

void InitScene(Engine& engine, Scene& scene)
{
    Importer importer(engine.GetDevice());

    // Axes
    auto boxX = importer.Import(scene, "models/BoxTextured.glb");
    boxX.SetPosition({ 5.0f, 1.5f, 0.0f });
    //dynamic_cast<PbrMaterial*>(boxX.Get<ModelInstance>().material)->baseColorFactor = Color::Red();

    auto boxZ = importer.Import(scene, "models/BoxTextured.glb");
    boxZ.SetPosition({ 0.0f, 1.5, 5.0f });
    //dynamic_cast<PbrMaterial*>(boxZ.Get<ModelInstance>().material)->baseColorFactor = Color::Blue();

    auto helmet = importer.Import(scene, "models/DamagedHelmet.glb");
    helmet.SetPosition(helmet.GetPosition() + Vector3::Up() + 3.0f * Vector3::Right());

    auto robo = importer.Import(scene, "models/robo/scene.gltf");
    robo.SetScale(0.2f);

    auto plane = importer.Import(scene, "models/Plane.glb");
    plane.SetScale(10.0f);

    auto model = plane.Get<ModelInstance>().model;
    auto pbr = dynamic_cast<PbrMaterial*>(model->begin()->material);
    pbr->roughnessFactor = 0.1;
    pbr->baseColorFactor = Color::Gray();

//    auto sponza = importer.Import(scene, "models/Sponza/Sponza.gltf");
//    sponza.SetScale(0.015f);

    HdrImporter hdrImporter(engine.GetDevice());
    scene.environment = hdrImporter.Import("textures/chinese_garden_4k.hdr");

    // Create camera entity
    scene.mainCamera = scene.CreateEntity();
    scene.mainCamera.Assign(Camera{ .verticalFov = kHalfPi, .aspectRatio = 1600.0f / 900.0f });
    scene.mainCamera.Assign(Transform{ .position = { 0.0f, 2.0f, 2.0f }});

    // Create directional light
    auto lightPos = Vector3(0.25f, 1.0f, 0.0f);

    scene.mainDirectionalLight = scene.CreateEntity();

    scene.mainDirectionalLight.Assign(DirectionalLight{
        .color = Color(201 / 255.0, 226 / 255.0, 255 / 255.0, 1.0),
        .cascades = {
            { .start = 0.0f, .end = 12.0f },
            { .start = 10.0f, .end = 32.0f },
            { .start = 30.0f, .end = 70.0f },
            { .start = 65.0f, .end = 150.0f }
        }
    });

    scene.mainDirectionalLight.Assign(Transform{ .position = lightPos });
    scene.mainDirectionalLight.SetRotation(Matrix4::Rotation(Matrix4::LookAt(lightPos, Vector3::Zero())));
}

int main()
{
    auto engine = Engine::Init();
    auto scene = engine->CreateScene();

    InitScene(*engine, *scene);
    while (engine->Update());

    return 0;
}

