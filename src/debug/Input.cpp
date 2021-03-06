#include "Input.hpp"

#include "GLFW/glfw3.h"

namespace lucent
{

Input::Input(GLFWwindow* window)
    : m_Window(window)
{
    glfwSetWindowUserPointer(window, this);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetCharCallback(window, CharCallback);

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    m_State.cursorPos = { (float)x, (float)y };
}

void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto& state = ((Input*)glfwGetWindowUserPointer(window))->m_State;

    if (key == GLFW_KEY_UNKNOWN || key >= state.keysDown.size()) return;

    state.keysDown[key] = action != GLFW_RELEASE;
    state.keysPressed[key] |= action == GLFW_PRESS;
}
void Input::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto& state = ((Input*)glfwGetWindowUserPointer(window))->m_State;

    auto pos = Vector2{ (float)xpos, (float)ypos };

    state.cursorDelta = pos - state.cursorPos;
    state.cursorPos = pos;
}

void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto& state = ((Input*)glfwGetWindowUserPointer(window))->m_State;

    if (button >= GLFW_MOUSE_BUTTON_LAST || button < 0 || button >= state.mouseButtonsDown.size()) return;

    state.mouseButtonsDown[button] = action != GLFW_RELEASE;
    state.mouseButtonsPressed[button] |= action == GLFW_PRESS;
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto& state = ((Input*)glfwGetWindowUserPointer(window))->m_State;

    state.scroll = { (float)xoffset, (float)yoffset };
}

void Input::CharCallback(GLFWwindow* window, uint32 codepoint)
{
    auto& state = ((Input*)glfwGetWindowUserPointer(window))->m_State;

    constexpr int kFirstAscii = 32;
    auto c = (char)codepoint;

    if (c >= kFirstAscii)
        state.textBuffer += c;
}

void Input::SetCursorVisible(bool visible)
{
    glfwSetInputMode(m_Window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

}
