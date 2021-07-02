#pragma once

#include "core/Math.hpp"
#include "core/Matrix4.hpp"
#include "core/Vector4.hpp"
#include "device/Device.hpp"
#include "rendering/SceneRenderer.hpp"
#include "scene/Scene.hpp"

namespace lucent::demos
{

class BasicDemo
{
public:
    void Init();
    void Draw(float dt);

public:
    Device m_Device {};
    std::unique_ptr<SceneRenderer> m_Renderer;

    Scene m_Scene{};
    Entity m_Player;
};

}