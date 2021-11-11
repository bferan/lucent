#pragma once

#include "scene/EntityIDPool.hpp"

namespace lucent
{

//! Transform component
struct Transform
{
public:
    Vector3 TransformDirection(Vector3 direction);
    Vector3 TransformPosition(Vector3 direction);

public:
    Quaternion rotation;
    Vector3 position;
    float scale = 1.0f;
    EntityID parent;
    Matrix4 model;
};

//! Parent component
struct Parent
{
    // TODO-OPT: Replace with small-size optimized vector
    std::vector<EntityID> children;
};

}