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
    bool Has();

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
    Matrix4 model;
};

struct Parent
{
    std::vector<EntityID> children;
};

void ApplyTransform(Entity entity);

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

    Texture* baseColorMap;
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

struct DirectionalLight
{

};

struct Scene
{
public:
    Scene();

    Entity CreateEntity();
    void Destroy(Entity entity);

    Entity Find(EntityID id);

    template<typename... Cs, typename F>
    void Each(F&& func);

public:
    EntityPool entities;

    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    Environment environment;

    Entity mainCamera;
    Entity mainDirectionalLight;

private:
    friend class Entity;
    std::vector<std::unique_ptr<ComponentPoolBase>> m_PoolsByIndex;

    template<typename T>
    ComponentPool<std::decay_t<T>>& GetPool();

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
bool Entity::Has()
{
    return scene->GetPool<T>().Contains(id);
}

template<typename T>
ComponentPool<std::decay_t<T>>& Scene::GetPool()
{
    using C = std::decay_t<T>;
    auto id = ComponentPool<C>::ID();
    ComponentPoolBase* pool = nullptr;

    if (id < m_PoolsByIndex.size())
    {
        pool = m_PoolsByIndex[id].get();
    }
    else
    {
        m_PoolsByIndex.resize(id + 1);
    }

    if (!pool)
    {
        pool = (m_PoolsByIndex[id] = std::make_unique<ComponentPool<C>>()).get();
    }
    return pool->As<C>();
}

template<typename... Cs, typename F>
void Scene::Each(F&& func)
{
    auto pools = std::tie(GetPool<Cs>()...);

    // Choose the smallest pool for iteration
    auto& pool = *std::min({ (ComponentPoolBase*)&std::get<ComponentPool<Cs>&>(pools)... }, [](auto* lhs, auto* rhs)
    {
        return lhs->Size() < rhs->Size();
    });

    for (auto id : pool)
    {
        if ((std::get<ComponentPool<Cs>&>(pools).Contains(id) && ...))
        {
            // Check if function can accept entity at compile time
            if constexpr (std::is_invocable_v<F, Entity, Cs...>)
            {
                func(Entity{ id, this }, std::get<ComponentPool<Cs>&>(pools)[id]...);
            }
            else
            {
                func(std::get<ComponentPool<Cs>&>(pools)[id]...);
            }
        }
    }
}


}