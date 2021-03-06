#include "DebugConsole.hpp"

#include "rendering/Engine.hpp"

using namespace std::literals;

namespace lucent
{

constexpr float kInitLifetime = 5.0f;
constexpr float kFadeTime = 1.0f;
const auto kPromptIndicator = "> "s;
constexpr char kPromptCursor = '_';
constexpr float kMaxScreenY = 800.0f;

DebugConsole::DebugConsole(Engine& engine, int maxColumns)
    : m_Engine(engine)
    , m_Device(engine.GetDevice())
    , m_Font(m_Device, "fonts/JetBrainsMono-Medium.ttf", 26.0f)
    , m_TextLog(m_Device, m_Font)
    , m_TextPrompt(m_Device, m_Font)
    , m_MaxColumns(maxColumns)
    , m_Active(false)
{
    Logger::Instance().Register(this);
    SetActive(m_Active);
}

void DebugConsole::GenerateMesh()
{
    m_TextLog.Clear();
    m_TextPrompt.Clear();

    const float lineHeight = m_Font.GetPixelHeight();
    const float entrySpacing = Round(0.25f * m_Font.GetPixelHeight());

    float y = m_Origin.y;
    float x = m_Origin.x;
    int col = 0;

    if (m_Active)
    {
        // Render text prompt
        auto indicatorColor = Color::Gray();
        x += m_TextPrompt.Draw(kPromptIndicator, x, y, indicatorColor);

        auto promptColor = Color::White();
        x += m_TextPrompt.Draw(m_Prompt.text, x, y, promptColor);

        m_TextPrompt.Draw(kPromptCursor, x, y, indicatorColor);
    }

    x = m_Origin.x;
    y += 2 * lineHeight;

    // Render text log
    for (auto& entry: m_Entries)
    {
        if (y > kMaxScreenY)
            break;

        auto offsetY = (float)(entry.lines - 1) * lineHeight;
        y += offsetY;

        auto color = entry.color;

        // Fade out new entries
        if (!m_Active)
            color.a *= Clamp(entry.lifetime, 0.0f, 1.0f);

        for (auto c: entry.text)
        {
            bool newLine = c == '\n';
            if (newLine || col == m_MaxColumns)
            {
                // Advance down a line
                x = m_Origin.x;
                y -= lineHeight;
                col = 0;
                if (newLine) continue;
            }
            x = Round(x);
            x += m_TextLog.Draw(c, x, y, color);
            ++col;
        }
        // Advance upward to next entry origin
        x = m_Origin.x;
        col = 0;
        y += offsetY + lineHeight + entrySpacing;
    }

    m_TextLog.Upload();
    m_TextPrompt.Upload();
}

void DebugConsole::AddEntry(std::string text, Color color)
{
    // Count lines according to column width
    int lines = 1, column = 0;
    for (char c: text)
    {
        if (c == '\n' || column == m_MaxColumns)
        {
            ++lines;
            column = 0;
        }
        else
        {
            ++column;
        }
    }

    auto& entry = m_Entries.emplace_front(DebugEntry{
        .text = std::move(text),
        .lines = lines,
        .color = color,
        .lifetime = kInitLifetime
    });

    if (m_Entries.size() > m_MaxEntries)
        m_Entries.pop_back();
}

void DebugConsole::Update(const InputState& input, float dt)
{
    // Update text prompt
    if (m_Active)
    {
        auto& text = m_Prompt.text;

        text += input.textBuffer;

        // Clear text with backspace
        if (input.KeyPressed(LC_KEY_BACKSPACE) && !text.empty())
        {
            if (input.KeyDown(LC_KEY_LEFT_CONTROL))
            {
                while (!text.empty() && text.back() == ' ') text.pop_back();
                while (!text.empty() && text.back() != ' ') text.pop_back();
            }
            else
            {
                text.pop_back();
            }
        }

        // Submit prompt text
        if (input.KeyPressed(LC_KEY_ENTER))
        {
            if (!text.empty())
                AddEntry(text);

            // TODO: Temporary commands, remove these or implement as more robust commands
            if (text == "q")
                exit(0);

            if (text == "r")
                m_Device->ReloadPipelines();

            SetActive(false);
        }

        // Close console with escape
        if (input.KeyPressed(LC_KEY_ESCAPE))
        {
            SetActive(false);
        }
    }
    else if (input.KeyPressed(LC_KEY_T) || input.KeyPressed(LC_KEY_ENTER))
    {
        SetActive(true);
    }

    // Fade out new entries
    for (auto& entry: m_Entries)
    {
        if (entry.lifetime <= 0.0f)
            break;

        entry.lifetime -= dt;
    }

    GenerateMesh();
}

void DebugConsole::RenderText(Context& ctx)
{
    m_TextLog.Render(ctx);
    m_TextPrompt.Render(ctx);
}

void DebugConsole::SetActive(bool active)
{
    m_Active = active;
    m_Prompt.text.clear();
    m_Engine.GetInput().SetCursorVisible(active);
}

void DebugConsole::OnLog(LogLevel level, const std::string& msg)
{
    AddEntry(msg);
}

void DebugConsole::SetScreenSize(uint32 width, uint32 height)
{
    m_TextLog.SetScreenSize(width, height);
    m_TextPrompt.SetScreenSize(width, height);
}

}
