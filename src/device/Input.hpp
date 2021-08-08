#pragma once

#include <array>
#include <string>

#include "device/Keys.hpp"
#include "core/Vector2.hpp"

struct GLFWwindow;

namespace lucent
{

struct InputState
{
    bool KeyDown(int key) const
    {
        return keysDown[key];
    }

    bool KeyPressed(int key) const
    {
        return keysPressed[key];
    }

    bool MouseButtonDown(int mouseButton) const
    {
        return mouseButtonsDown[mouseButton];
    }

    bool MouseButtonPressed(int mouseButton) const
    {
        return mouseButtonsPressed[mouseButton];
    }

    void Reset()
    {
        cursorDelta = { 0.0f, 0.0f };
        mouseButtonsPressed.fill(false);
        keysPressed.fill(false);
        textBuffer.clear();
    }

    Vector2 cursorPos{};
    Vector2 cursorDelta{};
    Vector2 scroll{};
    std::array<bool, LC_MOUSE_BUTTON_LAST> mouseButtonsDown{};
    std::array<bool, LC_MOUSE_BUTTON_LAST> mouseButtonsPressed{};
    std::array<bool, LC_KEY_LAST> keysDown{};
    std::array<bool, LC_KEY_LAST> keysPressed{};
    std::string textBuffer;
};

class Input
{
public:
    explicit Input(GLFWwindow* window);

    const InputState& GetState()
    {
        return m_State;
    }

    void SetCursorVisible(bool visible);

    // Call to clear deltas at end of frame
    void Reset()
    {
        m_State.Reset();
    }

private:
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void CharCallback(GLFWwindow* window, uint32_t codepoint);

private:
    GLFWwindow* m_Window;
    InputState m_State;

};

}
