#pragma once

#include <string>
#include <vector>

#include "device/Device.hpp"
#include "debug/Font.hpp"

namespace lucent
{

class TextMesh
{
public:
    TextMesh(Device* device, const Font& font);

    float Draw(const std::string& str, float x, float y, PackedColor color = Color::White().Pack());

    float Draw(char c, float screenX, float screenY, PackedColor color = Color::White().Pack());

    void Clear();

    void Upload();

    void Render(Context& context);

private:
    const Font& m_Font;
    bool m_Dirty;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    Device* m_Device;
    Buffer* m_VertexBuffer;
    Buffer* m_IndexBuffer;

};

}
