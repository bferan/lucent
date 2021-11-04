#include "Geometry.hpp"

#include "rendering/Mesh.hpp"

namespace lucent
{

using Vertex = Mesh::Vertex;

void InitGeometry(Device* device)
{
    // Create dummy textures
    uint32 black = 0xff000000;
    g_BlackTexture = device->CreateTexture(TextureSettings{});
    g_BlackTexture->Upload(sizeof(black), &black);

    uint32 white = 0xffffffff;
    g_WhiteTexture = device->CreateTexture(TextureSettings{});
    g_WhiteTexture->Upload(sizeof(white), &white);

    uint32 gray = 0xff808080;
    g_GrayTexture = device->CreateTexture(TextureSettings{});
    g_GrayTexture->Upload(sizeof(gray), &gray);

    uint32 normal = Color(0.5f, 0.5f, 1.0f).Pack();
    g_NormalTexture = device->CreateTexture(TextureSettings{});
    g_NormalTexture->Upload(sizeof(normal), &normal);

    uint32 green = 0xff00ff00;
    g_GreenTexture = device->CreateTexture(TextureSettings{});
    g_GreenTexture->Upload(sizeof(green), &green);

    // Cube
    {
        std::vector<Vertex> vertices;
        std::vector<uint32> indices;

        vertices.assign({
            { .position = { -1.0f, -1.0f, -1.0f }},
            { .position = { -1.0f, -1.0f, 1.0f }},
            { .position = { 1.0f, -1.0f, 1.0f }},
            { .position = { 1.0f, -1.0f, -1.0f }},
            { .position = { -1.0f, 1.0f, -1.0f }},
            { .position = { -1.0f, 1.0f, 1.0f }},
            { .position = { 1.0f, 1.0f, 1.0f }},
            { .position = { 1.0f, 1.0f, -1.0f }},
        });

        indices.assign({
            0, 1, 2, 2, 3, 0,
            0, 4, 5, 5, 1, 0,
            0, 3, 7, 7, 4, 0,
            2, 1, 5, 5, 6, 2,
            3, 2, 6, 6, 7, 3,
            4, 7, 6, 6, 5, 4
        });

        g_Cube.vertices = device->CreateBuffer(BufferType::kVertex, vertices.size() * sizeof(Vertex));
        g_Cube.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex), 0);

        g_Cube.indices = device->CreateBuffer(BufferType::kIndex, indices.size() * sizeof(uint32));
        g_Cube.indices->Upload(indices.data(), indices.size() * sizeof(uint32), 0);
        g_Cube.numIndices = indices.size();
    }

    // Quad
    {
        std::vector<Vertex> vertices;
        std::vector<uint32> indices;

        vertices.assign({
            { .position = { -1.0f, -1.0f, 0.0f }, .texCoord0 = { 0.0f, 0.0f }},
            { .position = { -1.0f, 1.0f, 0.0f }, .texCoord0 = { 0.0f, 1.0f }},
            { .position = { 1.0f, 1.0f, 0.0f }, .texCoord0 = { 1.0f, 1.0f }},
            { .position = { 1.0f, -1.0f, 0.0f }, .texCoord0 = { 1.0f, 0.0f }}
        });

        indices.assign({ 0, 1, 2, 2, 3, 0 });

        g_Quad.vertices = device->CreateBuffer(BufferType::kVertex, vertices.size() * sizeof(Vertex));
        g_Quad.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex), 0);

        g_Quad.indices = device->CreateBuffer(BufferType::kIndex, indices.size() * sizeof(uint32));
        g_Quad.indices->Upload(indices.data(), indices.size() * sizeof(uint32), 0);
        g_Quad.numIndices = indices.size();
    }

    // Sphere
    {
        std::vector<Vertex> vertices;
        std::vector<uint32> indices;

        constexpr int kNumSegments = 32;
        constexpr int kNumRings = 16;

        for (int ring = 0; ring <= kNumRings; ++ring)
        {
            auto theta = (float)ring * kPi / kNumRings;
            for (int segment = 0; segment < kNumSegments; ++segment)
            {
                auto phi = (float)segment * k2Pi / kNumSegments;
                auto pos = Vector3(Sin(theta) * Cos(phi), Cos(theta), Sin(theta) * Sin(phi));

                vertices.push_back(Vertex{
                    .position = pos,
                    .normal = pos,
                    .texCoord0 = { (float)segment / kNumSegments, (float)ring / kNumRings }
                });
            }
        }

        for (int ring = 1; ring <= kNumRings; ++ring)
        {
            for (int segment = 0; segment < kNumSegments; ++segment)
            {
                int start = ring * kNumSegments;
                int prev = start - kNumSegments;

                int pos = start + segment;

                indices.push_back(pos);
                indices.push_back(pos - kNumSegments);
                indices.push_back(((pos - kNumSegments + 1) % kNumSegments) + prev);

                indices.push_back(((pos - kNumSegments + 1) % kNumSegments) + prev);
                indices.push_back(((pos + 1) % kNumSegments) + start);
                indices.push_back(pos);
            }
        }

        g_Sphere.vertices = device->CreateBuffer(BufferType::kVertex, vertices.size() * sizeof(Vertex));
        g_Sphere.vertices->Upload(vertices.data(), vertices.size() * sizeof(Vertex), 0);

        g_Sphere.indices = device->CreateBuffer(BufferType::kIndex, indices.size() * sizeof(uint32));
        g_Sphere.indices->Upload(indices.data(), indices.size() * sizeof(uint32), 0);
        g_Sphere.numIndices = indices.size();
    }
}

}
