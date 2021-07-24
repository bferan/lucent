#include "DebugConsole.hpp"

#include "core/Lucent.hpp"
#include "core/Utility.hpp"

namespace lucent
{

DebugConsole::DebugConsole(Device* device)
    : m_Device(device)
    , m_Font(device, "fonts/JetBrainsMono-Medium.ttf", 26.0f)
    , m_TextMesh(device, m_Font)
{

    std::string test = "The quick brown fox jumps over the lazy dog. !!_$@#@!";

    float x = 50.0f;
    for (auto c : test)
    {
        x += m_TextMesh.Draw(c, x, 50.0f);
    }

    m_TextMesh.Upload();
}

void DebugConsole::Render(Context& ctx)
{
    m_TextMesh.Render(ctx);
}

DebugConsole::~DebugConsole()
{
}

}
