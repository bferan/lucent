#pragma once

namespace lucent
{

struct Camera
{
    Matrix4 GetProjectionMatrix() const;
    Matrix4 GetViewMatrix(Vector3 position) const;
    Matrix4 GetInverseViewMatrix(Vector3 position) const;

    float verticalFov = 1.0f;
    float aspectRatio = 1.0f;
    float near = 0.01f;
    float far = 10000.0f;

    float pitch = 0.0f;
    float yaw = 0.0f;
};

}