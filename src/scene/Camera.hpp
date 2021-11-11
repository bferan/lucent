#pragma once

namespace lucent
{

//! Camera component
struct Camera
{
public:
    Matrix4 GetProjectionMatrix() const;
    Matrix4 GetViewMatrix(Vector3 position) const;
    Matrix4 GetInverseViewMatrix(Vector3 position) const;

public:
    float verticalFov = 1.0f;
    float aspectRatio = 1.0f;
    float nearPlane = 0.01f;
    float farPlane = 10000.0f;

    // TODO: Remove debug camera controller data
    float pitch = 0.0f;
    float yaw = 0.0f;
};

}