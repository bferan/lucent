#pragma once

#include "device/Device.hpp"
#include "rendering/Renderer.hpp"
#include "debug/DebugConsole.hpp"

struct GLFWwindow;

namespace lucent
{

class Engine
{
public:
    Engine();

    bool Update();

    Device* GetDevice();

    DebugConsole* GetConsole();

    Scene* CreateScene();

    using BuildSceneRendererCallback = std::function<void(Engine*, Renderer&)>;

private:
    void UpdateDebug(float dt);

private:
    LogStdOut m_OutLog;
    std::unique_ptr<Device> m_Device;
    std::unique_ptr<DebugConsole> m_Console;
    std::unique_ptr<Input> m_Input;

    GLFWwindow* m_Window;

    std::unique_ptr<Renderer> m_SceneRenderer;
    BuildSceneRendererCallback m_BuildSceneRenderer;

    std::vector<std::unique_ptr<Scene>> m_Scenes;
    Scene* m_ActiveScene{};

    double m_LastUpdateTime{};
};

}
