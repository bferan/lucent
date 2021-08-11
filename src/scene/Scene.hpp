#pragma once

#include "scene/EntityPool.hpp"
#include "scene/ComponentPool.hpp"

#include "device/Device.hpp"

namespace lucent
{

class Scene;

struct Entity
{
    template<typename T>
    T& Get();

    template<typename T>
    void Assign(T&& component);

    template<typename T>
    void Remove();

public:
    EntityID id;
    Scene* scene{};
};

struct Transform
{
    Quaternion rotation; // 16
    Vector3 position;    // 12
    float scale = 1.0f;         // 4
    EntityID parent;       // 4
};

struct Parent
{
    explicit Parent(size_t numChildren)
        : children(numChildren)
    {}

    std::vector<EntityID> children;
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
public:
    Scene();

    Entity Create();
    void Destroy(Entity entity);

public:
    EntityPool entities;

    ComponentPool<Transform> transforms;
    ComponentPool<Parent> parents;
    ComponentPool<MeshInstance> meshInstances;
    ComponentPool<Camera> cameras;

    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    Environment environment;

private:
    friend class Entity;
    std::vector<ComponentPoolBase*> m_PoolsByIndex;

    template<typename T>
    void RegisterPool(ComponentPool<T>& pool);

    template<typename T>
    ComponentPool<T>& GetPool();

};

template<typename T>
T& Entity::Get()
{
    return scene->template GetPool<T>()[id];
}

template<typename T>
void Entity::Assign(T&& component)
{
    scene->GetPool<T>().Assign(id, std::forward<T>(component));
}

template<typename T>
void Entity::Remove()
{
    scene->GetPool<T>().Remove(id);
}

template<typename T>
void Scene::RegisterPool(ComponentPool<T>& pool)
{
    auto id = ComponentPool<T>::ID();
    if (id >= m_PoolsByIndex.size())
    {
        m_PoolsByIndex.resize(id + 1);
    }
    m_PoolsByIndex[id] = &pool;
}

template<typename T>
ComponentPool<T>& Scene::GetPool()
{
    auto id = ComponentPool<T>::ID();
    ComponentPoolBase* pool = nullptr;

    if (id < m_PoolsByIndex.size())
        pool = m_PoolsByIndex[id];

    if (!pool)
    {
        LC_ASSERT(0 && "Unable to find component pool");
    }
    return pool->As<T>();
}

}