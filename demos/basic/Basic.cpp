#include "Engine.hpp"
#include "scene/Importer.hpp"
#include "scene/HdrImporter.hpp"

using namespace lucent;

void InitScene(Engine& engine, Scene& scene)
{
    Importer importer(engine.Device());

    // Axes
    auto boxX = importer.Import(scene, "models/BoxTextured.glb");
    boxX.Get<Transform>().position = Vector3(5.0f, 0.5, 0.0f);
    ApplyTransform(boxX);
    scene.materials[0].baseColorFactor = Color::Red();

    auto boxZ = importer.Import(scene, "models/BoxTextured.glb");
    boxZ.Get<Transform>().position = Vector3(0.0f, 0.5, 5.0f);
    ApplyTransform(boxZ);
    scene.materials[1].baseColorFactor = Color::Blue();

    auto helmet = importer.Import(scene, "models/DamagedHelmet.glb");
    helmet.Get<Transform>().position += Vector3::Up();
    ApplyTransform(helmet);

//        auto plane = importer.Import(m_Scene, "models/Plane.glb");
//        auto& planeTransform = plane.Get<Transform>();
//        planeTransform.scale = 10.0f;
//        ApplyTransform(plane);
//
//        auto& mesh = m_Scene.meshes[plane.Get<MeshInstance>().meshes[0]];
//        auto& mat = m_Scene.materials[mesh.materialIdx];
//
//        mat.baseColorMap = g_WhiteTexture;
//        mat.metalRough = g_WhiteTexture;
//        mat.baseColorFactor = Color::White();
//        mat.roughnessFactor = 0.0;
//        mat.metallicFactor = 1.0;

    auto sponza = importer.Import(scene, "models/Sponza/Sponza.gltf");
    sponza.Get<Transform>().scale = 0.015f;
    ApplyTransform(sponza);

    HdrImporter hdrImporter(engine.Device());
    scene.environment = hdrImporter.Import("textures/chinese_garden_4k.hdr");

    // Create camera entity
    scene.mainCamera = scene.CreateEntity();
    scene.mainCamera.Assign(Camera{ .verticalFov = kHalfPi, .aspectRatio = 1600.0f / 900.0f });
    scene.mainCamera.Assign(Transform{ .position = { 0.0f, 2.0f, 2.0f }});

    // Create directional light
    auto lightPos = Vector3(0.25f, 1.0f, 0.0f);

    scene.mainDirectionalLight = scene.CreateEntity();

    scene.mainDirectionalLight.Assign(DirectionalLight{
        .cascades = {
            { .start = 0.0f, .end = 12.0f },
            { .start = 10.0f, .end = 32.0f },
            { .start = 30.0f, .end = 70.0f },
            { .start = 65.0f, .end = 150.0f }
        }
    });

    scene.mainDirectionalLight.Assign(Transform{
        .rotation = Matrix4::Rotation(Matrix4::LookAt(lightPos, Vector3::Zero())),
        .position = lightPos
    });
}

int main()
{
    Engine engine;
    auto scene = engine.CreateScene();

    InitScene(engine, *scene);
    while (engine.Update());

    return 0;
}

