#pragma once

#include "rendering/Model.hpp"
#include "rendering/Material.hpp"
#include "scene/Entity.hpp"
#include "scene/EntityIDPool.hpp"
#include "scene/ComponentPool.hpp"
#include "scene/Lighting.hpp"
#include "device/Device.hpp"

namespace lucent
{

class Scene
{
public:
    Entity CreateEntity();
    void Destroy(Entity entity);

    Entity Find(EntityID id);

    //! Iterate over all entities with given components
    template<typename... Cs, typename F>
    void Each(F&& func);

    //! Add a model to the scene (scene takes ownership)
    Model* AddModel(std::unique_ptr<Model> model);

    //! Add a material to the scene (scene takes ownership)
    Material* AddMaterial(std::unique_ptr<Material> material);

    Material* GetDefaultMaterial() const;

public:
    Entity mainCamera;
    Entity mainDirectionalLight;
    Environment environment;

private:
    friend class Entity;

    template<typename T>
    ComponentPool<std::decay_t<T>>& GetPool();

private:
    EntityIDPool m_Entities;
    std::vector<std::unique_ptr<ComponentPoolBase>> m_ComponentPoolsByIndex;

    std::vector<std::unique_ptr<Model>> m_Models;
    std::vector<std::unique_ptr<Material>> m_Materials;
};

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

/* Entity implementation */
template<typename T>
T& Entity::Get()
{
    return scene->template GetPool<T>()[id];
}

template<typename T>
const T& Entity::Get() const
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
bool Entity::Has() const
{
    return scene->GetPool<T>().Contains(id);
}


}