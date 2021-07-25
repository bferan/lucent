#include "Input.hpp"

#include "GLFW/glfw3.h"

namespace lucent
{

// TODO: Flip mouse so screen space has origin at bottom left

Input::Input(GLFWwindow* window)
{
    glfwSetWindowUserPointer(window, this);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    if (key == GLFW_KEY_UNKNOWN) return;

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

    if (button >= GLFW_MOUSE_BUTTON_LAST || button < 0) return;

    state.mouseButtonsDown[button] = action != GLFW_RELEASE;
    state.mouseButtonsPressed[button] |= action == GLFW_PRESS;
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto& state = ((Input*)glfwGetWindowUserPointer(window))->m_State;

    state.scroll = { (float)xoffset, (float)yoffset };
}

void Input::CharCallback(GLFWwindow* window, uint32_t codepoint)
{
    auto& state = ((Input*)glfwGetWindowUserPointer(window))->m_State;

    constexpr int kFirstAscii = 32;
    auto c = (char)codepoint;

    if (c >= kFirstAscii)
        state.textBuffer += c;
}

}
