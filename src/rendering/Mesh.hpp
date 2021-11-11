#pragma once

namespace lucent
{

//! Wraps dynamic mesh data which can be uploaded and baked into a StaticMesh
class Mesh
{
public:
    struct Vertex
    {
        Vector3 position;
        Vector3 normal;
        Vector4 tangent;
        Vector2 texCoord0;
        Color color = Color::White();
    };

    void Clear();

    void Reserve(uint32 numVertices, uint32 numIndices)
    {
        vertices.reserve(numVertices);
        indices.reserve(numIndices);
    }

    void AddVertex(Vertex vertex)
    {
        vertices.push_back(vertex);
    };

    void AddIndex(uint32 index)
    {
        indices.push_back(index);
    }

public:
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

};

}
