#include "scene/Transform.hpp"

namespace lucent
{

Vector3 Transform::TransformDirection(Vector3 dir)
{
    return Vector3(model * Vector4(dir, 0.0));
}

Vector3 Transform::TransformPosition(Vector3 dir)
{
    return Vector3(model * Vector4(dir, 1.0));
}

}