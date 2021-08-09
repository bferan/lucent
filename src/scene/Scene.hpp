#pragma once

#include "scene/EntityPool.hpp"
#include "scene/ComponentPool.hpp"

#include "device/Device.hpp"

namespace lucent
{

struct Transform
{
    Quaternion rotation; // 16
    Vector3 position;    // 12
    float scale = 1.0f;         // 4
    Entity parent;       // 4
};

struct Parent
{
    explicit Parent(size_t numChildren)
        : children(numChildren)
    {}

    std::vector<Entity> children;
};

struct MeshInstance
{
    std::vector<uint32> meshes;
};

struct Mesh
{
    Buffer* vertexBuffer;
    Buffer* indexBuffer;

    uint32 numIndices;

    uint32 materialIdx;
};

struct Material
{
    Color baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    Texture* baseColor;
    Texture* metalRough;
    Texture* normalMap;
    Texture* aoMap;
    Texture* emissive;
};

struct Camera
{
    float horizontalFov = 1.0f;
    float aspectRatio = 1.0f;
    float near = 0.01f;
    float far = 10000.0f;

    float pitch = 0.0f;
    float yaw = 0.0f;
};

struct Environment
{
    Texture* cubeMap;
    Texture* irradianceMap;
    Texture* specularMap;
    Texture* BRDF;
};

struct Scene
{
    EntityPool entities;

    ComponentPool<Transform> transforms;
    ComponentPool<Parent> parents;
    ComponentPool<MeshInstance> meshInstances;
    ComponentPool<Camera> cameras;

    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    Environment environment;
};

}