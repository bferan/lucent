#pragma once

#include "scene/Scene.hpp"

namespace lucent
{

class View
{
public:
    void SetScene(Scene* scene);

    Scene& GetScene();

    const Matrix4& GetViewMatrix() const;

    const Matrix4& GetInverseViewMatrix() const;

    const Matrix4& GetProjectionMatrix() const;

    const Matrix4& GetViewProjectionMatrix() const;

    void BindUniforms(Context& ctx) const;

private:
    Scene* m_Scene{};

    Matrix4 m_View;
    Matrix4 m_ViewInverse;
    Matrix4 m_Projection;
    Matrix4 m_ViewProjection;

    // Uniforms
    Vector4 m_ScreenToView;
    float m_AspectRatio;
};

}
