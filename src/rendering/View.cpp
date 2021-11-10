#include "View.hpp"

#include "scene/Camera.hpp"

namespace lucent
{

void View::SetScene(Scene* scene)
{
    m_Scene = scene;

    auto& camera = scene->mainCamera.Get<Camera>();
    auto camPos = scene->mainCamera.GetPosition();

    m_View = camera.GetViewMatrix(camPos);
    m_ViewInverse = camera.GetInverseViewMatrix(camPos);
    m_Projection = camera.GetProjectionMatrix();
    m_ViewProjection = m_Projection * m_View;

    m_ScreenToView = Vector4(
        2.0f * (1.0f / m_Projection(0, 0)),
        2.0f * (1.0f / m_Projection(1, 1)),
        m_Projection(2, 2), m_Projection(2, 3));

    m_AspectRatio = m_Projection(1, 1) / m_Projection(0, 0);
}

void View::BindUniforms(Context& ctx) const
{
    ctx.Uniform("u_ScreenToView"_id, m_ScreenToView);
    ctx.Uniform("u_ViewToWorld"_id, m_ViewInverse);
    ctx.Uniform("u_WorldToView"_id, m_View);
    ctx.Uniform("u_ViewToScreen"_id, m_Projection);
    ctx.Uniform("u_AspectRatio"_id, m_AspectRatio);
}

Scene& View::GetScene()
{
    LC_ASSERT(m_Scene);
    return *m_Scene;
}

const Matrix4& View::GetViewMatrix() const
{
    return m_View;
}

const Matrix4& View::GetInverseViewMatrix() const
{
    return m_ViewInverse;
}
const Matrix4& View::GetProjectionMatrix() const
{
    return m_Projection;
}

const Matrix4& View::GetViewProjectionMatrix() const
{
    return m_ViewProjection;
}

}
