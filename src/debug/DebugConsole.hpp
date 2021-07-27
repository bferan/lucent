#pragma once

#include <deque>

#include "core/Color.hpp"
#include "core/Log.hpp"
#include "device/Device.hpp"
#include "device/Input.hpp"
#include "debug/TextMesh.hpp"

namespace lucent
{

struct DebugEntry
{
    std::string text;
    int lines;
    Color color;
    float lifetime;
};

struct DebugPrompt
{
    std::string text;
};

class DebugConsole : public LogListener
{
public:
    explicit DebugConsole(Device* device, int maxColumns = 120);

    void AddEntry(std::string text, Color color = Color::White());

    void Update(const InputState& input, float dt);

    void Render(Context& ctx);

    bool Active() const
    {
        return m_Active;
    }

    void OnLog(LogLevel level, const std::string& msg) override;

    void GenerateMesh();

private:
    Font m_Font;
    TextMesh m_TextLog;
    TextMesh m_TextPrompt;

    int m_MaxColumns;
    int m_MaxEntries = 1000;
    bool m_Active;
    Vector2 m_Origin = { 50.0f, 50.0f };

    std::deque<DebugEntry> m_Entries;
    DebugPrompt m_Prompt;

};

}
