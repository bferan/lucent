#pragma once

#include "scene/EntityIDPool.hpp"
#include "scene/ComponentPool.hpp"

namespace lucent
{
class Scene;

class Entity
{
public:
    template<typename Component>
    Component& Get();

    template<typename Component>
    const Component& Get() const;

    template<typename Component>
    bool Has() const;

    template<typename Component>
    void Assign(Component&& component);

    template<typename Component>
    void Remove();

public:
    // Convenience accessors:
    void SetPosition(Vector3 position);
    Vector3 GetPosition() const;

    void SetRotation(Quaternion rotation);
    Quaternion GetRotation() const;

    void SetScale(float scale);
    float GetScale() const;

    void SetTransform(Vector3 position, Quaternion rotation, float scale);

public:
    EntityID id;
    Scene* scene{};
};


}