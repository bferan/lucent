#pragma once

#include <vector>

#include "core/Lucent.hpp"
#include "core/Matrix4.hpp"
#include "core/Vector3.hpp"
#include "core/Quaternion.hpp"
#include "scene/EntityPool.hpp"
#include "scene/ComponentPool.hpp"

#include "device/Device.hpp"

namespace lucent
{

struct Transform
{
    Quaternion rotation; // 16
    Vector3 position;    // 12
    float scale;         // 4
    Entity parent;       // 4
};

struct Parent
{
    std::vector<Entity> children;
};

struct MeshInstance
{
    uint32_t meshIndex;
};

struct Mesh
{
    Buffer* vertexBuffer;
    Buffer* indexBuffer;

    uint32_t numIndices;

    DescriptorSet* descSet;
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

struct Scene
{
    EntityPool entities;

    ComponentPool<Transform> transforms;
    ComponentPool<Parent> parents;
    ComponentPool<MeshInstance> meshInstances;
    ComponentPool<Camera> cameras;

    std::vector<Mesh> meshes;
};

}