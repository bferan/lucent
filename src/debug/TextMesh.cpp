#include "TextMesh.hpp"

#include "device/Context.hpp"

namespace lucent
{

constexpr size_t kVertBufferSize = (1 << 20);
constexpr size_t kIdxBufferSize = (1 << 20);

TextMesh::TextMesh(Device* device, const Font& font)
    : m_Font(font), m_Device(device), m_Dirty(true)
{
    m_VertexBuffer = m_Device->CreateBuffer(BufferType::kVertex, kVertBufferSize);
    m_IndexBuffer = m_Device->CreateBuffer(BufferType::kIndex, kIdxBufferSize);
}

float TextMesh::Draw(const std::string& str, float x, float y, Color color)
{
    float originX = x;

    for (auto c : str)
    {
        x = Round(x);
        x += Draw(c, x, y, color);
    }
    return x - originX;
}

float TextMesh::Draw(char c, float screenX, float screenY, Color color)
{
    const auto& glyph = m_Font.GetGlyph(c);

    const auto invScreen = Vector2(1.0f / (float)m_ScreenWidth, 1.0f / (float)m_ScreenHeight);

    auto origin = Vector2(screenX, screenY);

    // Convert to screen coordinates
    auto min = 2.0f * ((origin + glyph.minPos) * invScreen) - Vector2::One();
    auto max = 2.0f * ((origin + glyph.maxPos) * invScreen) - Vector2::One();
    auto& minTC = glyph.minTexCoord;
    auto& maxTC = glyph.maxTexCoord;

    auto idx = m_Vertices.size();

    m_Vertices.push_back({ .position = { min.x, min.y }, . texCoord0 = { minTC.x, minTC.y }, .color = color });
    m_Vertices.push_back({ .position = { max.x, min.y }, . texCoord0 = { maxTC.x, minTC.y }, .color = color });
    m_Vertices.push_back({ .position = { max.x, max.y }, . texCoord0 = { maxTC.x, maxTC.y }, .color = color });
    m_Vertices.push_back({ .position = { min.x, max.y }, . texCoord0 = { minTC.x, maxTC.y }, .color = color });

    m_Indices.push_back(idx + 0);
    m_Indices.push_back(idx + 1);
    m_Indices.push_back(idx + 2);
    m_Indices.push_back(idx + 2);
    m_Indices.push_back(idx + 3);
    m_Indices.push_back(idx + 0);

    m_Dirty = true;
    return glyph.advance;
}

void TextMesh::Clear()
{
    m_Vertices.clear();
    m_Indices.clear();

    m_Dirty = true;
}

void TextMesh::Upload()
{
    if (!m_Dirty) return;

    m_VertexBuffer->Upload(m_Vertices.data(), m_Vertices.size() * sizeof(Mesh::Vertex), 0);
    m_IndexBuffer->Upload(m_Indices.data(), m_Indices.size() * sizeof(uint32), 0);

    m_Dirty = false;
}

void TextMesh::Render(Context& context)
{
    context.BindTexture("u_FontAtlas"_id, m_Font.GetAtlas());
    context.BindBuffer(m_IndexBuffer);
    context.BindBuffer(m_VertexBuffer);
    context.Draw(m_Indices.size());
}

void TextMesh::SetScreenSize(uint32 width, uint32 height)
{
    m_ScreenWidth = width;
    m_ScreenHeight = height;
}

}
