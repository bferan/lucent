#pragma once

#include "device/Device.hpp"
#include "rendering/Mesh.hpp"

namespace lucent
{

//! Handle to a fixed size GPU-resident mesh
class StaticMesh
{
public:
    StaticMesh(Device* device, const Mesh& mesh);

    StaticMesh(const StaticMesh&) = delete;
    StaticMesh& operator=(const StaticMesh&) = delete;

    StaticMesh(StaticMesh&& mesh) noexcept ;
    StaticMesh& operator=(StaticMesh&& mesh) noexcept;

    ~StaticMesh();

public:
    Device* device;
    Buffer* vertexBuffer;
    Buffer* indexBuffer;
    uint32 numIndices;
};

}
