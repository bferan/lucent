#include "scene/Components.hpp"

namespace lucent
{

Matrix4 Camera::GetProjectionMatrix() const
{
    return Matrix4::Perspective(verticalFov, aspectRatio, near, far);
}

Matrix4 Camera::GetViewMatrix(Vector3 position) const
{
    return Matrix4::RotationX(kPi) *
        Matrix4::RotationX(-pitch) *
        Matrix4::RotationY(-yaw) *
        Matrix4::Translation(-position);
}

Matrix4 Camera::GetInverseViewMatrix(Vector3 position) const
{
    return Matrix4::Translation(position) *
        Matrix4::RotationY(yaw) *
        Matrix4::RotationX(pitch) *
        Matrix4::RotationX(kPi); // Flip axes
}

Vector3 Transform::TransformDirection(Vector3 dir)
{
    return Vector3(model * Vector4(dir, 0.0));
}

Vector3 Transform::TransformPosition(Vector3 dir)
{
    return Vector3(model * Vector4(dir, 1.0));
}

}

