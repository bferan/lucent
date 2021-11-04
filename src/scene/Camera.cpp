#include "scene/Camera.hpp"

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

}