#pragma once

#include "device/Device.hpp"
#include "rendering/Renderer.hpp"
#include "debug/DebugConsole.hpp"

namespace lucent
{
class Window;

//! Entrypoint for the rendering engine
class Engine
{
public:
    static Engine* Init(RenderSettings settings = {});

    static Engine* Instance();

    bool Update();

    Device* GetDevice();

    DebugConsole* GetConsole();

    Scene* CreateScene();

    Input& GetInput();

    const RenderSettings& GetRenderSettings();

    using BuildSceneRendererCallback = std::function<void(Engine*, Renderer&)>;

private:
    explicit Engine(RenderSettings settings);

    void UpdateDebug(float dt);

private:
    static std::unique_ptr<Engine> s_Engine;

    LogStdOut m_OutLog;
    std::unique_ptr<Device> m_Device;
    std::unique_ptr<DebugConsole> m_Console;
    std::unique_ptr<Input> m_Input;
    std::unique_ptr<Window> m_Window;

    std::unique_ptr<Renderer> m_SceneRenderer;
    BuildSceneRendererCallback m_BuildSceneRenderer;

    std::vector<std::unique_ptr<Scene>> m_Scenes;
    Scene* m_ActiveScene{};

    double m_LastUpdateTime{};
};

}
