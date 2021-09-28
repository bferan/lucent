#pragma once

#include "device/Texture.hpp"
#include "device/Buffer.hpp"
#include "scene/EntityIDPool.hpp"

namespace lucent
{

struct Transform
{
    Quaternion rotation; // 16
    Vector3 position;    // 12
    float scale = 1.0f;         // 4
    EntityID parent;       // 4
    Matrix4 model;

    Vector3 TransformDirection(Vector3 dir);
    Vector3 TransformPosition(Vector3 dir);
};

struct Parent
{
    std::vector<EntityID> children;
};

struct MeshInstance
{
    std::vector<uint32> meshes;
};

struct Mesh
{
    Buffer* vertexBuffer;
    Buffer* indexBuffer;

    uint32 numIndices;

    uint32 materialIdx;
};

struct Material
{
    Color baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    Texture* baseColorMap;
    Texture* metalRough;
    Texture* normalMap;
    Texture* aoMap;
    Texture* emissive;
};

struct Camera
{
    Matrix4 GetProjectionMatrix() const;
    Matrix4 GetViewMatrix(Vector3 position) const;
    Matrix4 GetInverseViewMatrix(Vector3 position) const;

    float verticalFov = 1.0f;
    float aspectRatio = 1.0f;
    float near = 0.01f;
    float far = 10000.0f;

    float pitch = 0.0f;
    float yaw = 0.0f;
};

struct DirectionalLight
{
    Color color;

    static constexpr int kNumCascades = 4;
    static constexpr int kMapWidth = 2048;

    struct Cascade
    {
        float start;
        float end;

        float worldSpaceTexelSize;

        Vector3 pos;
        float width;
        float depth;
        Matrix4 proj;

        Vector3 offset;
        Vector3 scale;
        Vector4 frontPlane;
    };

    Cascade cascades[kNumCascades];
};

struct Environment
{
    Texture* cubeMap;
    Texture* irradianceMap;
    Texture* specularMap;
    Texture* BRDF;
};

}