#include "Scene.hpp"

namespace lucent
{

Scene::Scene()
{
}

Entity Scene::CreateEntity()
{
    return Entity{ entities.Create(), this };
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

Entity Scene::Find(EntityID id)
{
    return Entity{ id, this };
}

void ApplyTransform(Entity entity)
{
    auto& transform = entity.Get<Transform>();

    transform.model = Matrix4::Translation(transform.position) *
        Matrix4::Rotation(transform.rotation) *
        Matrix4::Scale(transform.scale, transform.scale, transform.scale);

    if (!transform.parent.Empty())
    {
        auto& parent = entity.scene->Find(transform.parent).Get<Transform>();
        transform.model = parent.model * transform.model;
    }

    if (entity.Has<Parent>())
    {
        for (auto id : entity.Get<Parent>().children)
        {
            ApplyTransform(entity.scene->Find(id));
        }
    }
}

}