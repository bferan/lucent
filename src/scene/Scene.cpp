#include "Scene.hpp"

namespace lucent
{

Scene::Scene()
{
}

Entity Scene::CreateEntity()
{
    return Entity { entities.Create(), this };
}

void Scene::Destroy(Entity entity)
{
    for (auto& pool : m_PoolsByIndex)
    {
        if (pool && pool->Contains(entity.id))
            pool->Remove(entity.id);
    }
    entities.Destroy(entity.id);
}

}