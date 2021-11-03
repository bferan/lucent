#pragma once

#include "scene/EntityIDPool.hpp"
#include "scene/ComponentPool.hpp"
#include "scene/Components.hpp"
#include "device/Device.hpp"

namespace lucent
{

class Scene;

class Entity
{
public:
    template<typename Component>
    Component& Get();

    template<typename Component>
    bool Has();

    template<typename Component>
    void Assign(Component&& component);

    template<typename Component>
    void Remove();

public:
    // Convenience accessors:
    void SetPosition(Vector3 position);
    Vector3 GetPosition(); // TODO: Make const

    void SetRotation(Quaternion rotation);
    Quaternion GetRotation();

    void SetScale(float scale);
    float GetScale();

    void SetTransform(Vector3 position, Quaternion rotation, float scale);

public:
    EntityID id;
    Scene* scene{};
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
    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    Environment environment;

    Entity mainCamera;
    Entity mainDirectionalLight;

private:
    friend class Entity;

    EntityIDPool m_Entities;
    std::vector<std::unique_ptr<ComponentPoolBase>> m_ComponentPoolsByIndex;

    template<typename T>
    ComponentPool<std::decay_t<T>>& GetPool();

};

/* Entity implementation */
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

/* Scene implementation */
template<typename T>
ComponentPool<std::decay_t<T>>& Scene::GetPool()
{
    using C = std::decay_t<T>;
    auto id = ComponentPool<C>::ID();
    ComponentPoolBase* pool = nullptr;

    if (id < m_ComponentPoolsByIndex.size())
    {
        pool = m_ComponentPoolsByIndex[id].get();
    }
    else
    {
        m_ComponentPoolsByIndex.resize(id + 1);
    }

    if (!pool)
    {
        pool = (m_ComponentPoolsByIndex[id] = std::make_unique<ComponentPool<C>>()).get();
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

    for (auto id: pool)
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