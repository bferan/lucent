#include "scene/Transform.hpp"

namespace lucent
{

Vector3 Transform::TransformDirection(Vector3 direction)
{
    return Vector3(model * Vector4(direction, 0.0));
}

Vector3 Transform::TransformPosition(Vector3 direction)
{
    return Vector3(model * Vector4(direction, 1.0));
}

}