#include "scene/Entity.hpp"
#include "scene/Transform.hpp"
#include "scene/Scene.hpp"

namespace lucent
{

/* Entity implementation */
static void ApplyTransform(Entity entity)
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

void Entity::SetPosition(Vector3 position)
{
    Get<Transform>().position = position;
    ApplyTransform(*this);
}

Vector3 Entity::GetPosition() const
{
    return Get<Transform>().position;
}

void Entity::SetRotation(Quaternion rotation)
{
    Get<Transform>().rotation = rotation;
    ApplyTransform(*this);
}

Quaternion Entity::GetRotation() const
{
    return Get<Transform>().rotation;
}

void Entity::SetScale(float scale)
{
    Get<Transform>().scale = scale;
    ApplyTransform(*this);
}

float Entity::GetScale() const
{
    return Get<Transform>().scale;
}

void Entity::SetTransform(Vector3 position, Quaternion rotation, float scale)
{
    auto& transform = Get<Transform>();
    transform.position = position;
    transform.rotation = rotation;
    transform.scale = scale;
    ApplyTransform(*this);
}


}