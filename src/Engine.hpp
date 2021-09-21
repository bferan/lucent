#pragma once

#include "device/Device.hpp"
#include "rendering/SceneRenderer.hpp"

struct GLFWwindow;

namespace lucent
{

class Engine
{
public:
    Engine();

    bool Update();

    Device* Device();

    Scene* CreateScene();

private:
    void UpdateDebug(float dt);

private:
    std::unique_ptr<lucent::Device> m_Device;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    GLFWwindow* m_Window;

    std::vector<std::unique_ptr<Scene>> m_Scenes;
    Scene* m_ActiveScene{};

    double m_LastUpdateTime{};
};

}
