#pragma once

#include "scene/EntityIDPool.hpp"

namespace lucent
{

struct Transform
{
    Quaternion rotation; // 16
    Vector3 position;    // 12
    float scale = 1.0f;         // 4
    EntityID parent;       // 4
    Matrix4 model;

    Vector3 TransformDirection(Vector3 dir);
    Vector3 TransformPosition(Vector3 dir);
};

struct Parent
{
    std::vector<EntityID> children;
};

}