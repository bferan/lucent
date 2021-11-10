#include "StaticMesh.hpp"

namespace lucent
{

StaticMesh::StaticMesh(Device* dev, const Mesh& mesh)
    : device(dev)
{
    VkDeviceSize vertSize = mesh.vertices.size() * sizeof(Mesh::Vertex);
    VkDeviceSize indexSize = mesh.indices.size() * sizeof(uint32);

    vertexBuffer = device->CreateBuffer(BufferType::kVertex, vertSize);
    indexBuffer = device->CreateBuffer(BufferType::kIndex, indexSize);
    numIndices = mesh.indices.size();

    vertexBuffer->Upload(mesh.vertices.data(), vertSize, 0);
    indexBuffer->Upload(mesh.indices.data(), indexSize, 0);
}

StaticMesh::StaticMesh(StaticMesh&& mesh) noexcept
    : device(mesh.device)
    , vertexBuffer(mesh.vertexBuffer)
    , indexBuffer(mesh.indexBuffer)
    , numIndices(mesh.numIndices)
{
    mesh.device = nullptr;
}

StaticMesh& StaticMesh::operator=(StaticMesh&& mesh) noexcept
{
    device = mesh.device;
    vertexBuffer = mesh.vertexBuffer;
    indexBuffer = mesh.indexBuffer;
    numIndices = mesh.numIndices;

    mesh.device = nullptr;
    return *this;
}

StaticMesh::~StaticMesh()
{
    if (device)
    {
        device->WaitIdle();
        device->DestroyBuffer(vertexBuffer);
        device->DestroyBuffer(indexBuffer);
    }
}

}
