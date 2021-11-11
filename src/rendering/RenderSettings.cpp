#include "RenderSettings.hpp"

namespace lucent
{

std::pair<uint32, uint32> RenderSettings::ComputeGroupCount(uint32 width, uint32 height) const
{
    auto x = (uint32)Ceil((float)width / (float)defaultGroupSizeX);
    auto y = (uint32)Ceil((float)height / (float)defaultGroupSizeY);
    return { x, y };
}


// Default mesh helpers:

static Mesh CreateCube()
{
    Mesh cube;
    cube.vertices.assign({
        { .position = { -1.0f, -1.0f, -1.0f }},
        { .position = { -1.0f, -1.0f, 1.0f }},
        { .position = { 1.0f, -1.0f, 1.0f }},
        { .position = { 1.0f, -1.0f, -1.0f }},
        { .position = { -1.0f, 1.0f, -1.0f }},
        { .position = { -1.0f, 1.0f, 1.0f }},
        { .position = { 1.0f, 1.0f, 1.0f }},
        { .position = { 1.0f, 1.0f, -1.0f }},
    });
    cube.indices.assign({
        0, 1, 2, 2, 3, 0,
        0, 4, 5, 5, 1, 0,
        0, 3, 7, 7, 4, 0,
        2, 1, 5, 5, 6, 2,
        3, 2, 6, 6, 7, 3,
        4, 7, 6, 6, 5, 4
    });
    return cube;
}

static Mesh CreateQuad()
{
    Mesh quad;
    quad.vertices.assign({
        { .position = { -1.0f, -1.0f, 0.0f }, .texCoord0 = { 0.0f, 0.0f }},
        { .position = { -1.0f, 1.0f, 0.0f }, .texCoord0 = { 0.0f, 1.0f }},
        { .position = { 1.0f, 1.0f, 0.0f }, .texCoord0 = { 1.0f, 1.0f }},
        { .position = { 1.0f, -1.0f, 0.0f }, .texCoord0 = { 1.0f, 0.0f }}
    });
    quad.indices.assign({ 0, 1, 2, 2, 3, 0 });
    return quad;
}

static Mesh CreateSphere(int numSegments = 32, int numRings = 16)
{
    Mesh sphere;
    for (int ring = 0; ring <= numRings; ++ring)
    {
        auto theta = (float)ring * kPi / (float)numRings;
        for (int segment = 0; segment < numSegments; ++segment)
        {
            auto phi = (float)segment * k2Pi / (float)numSegments;
            auto pos = Vector3(Sin(theta) * Cos(phi), Cos(theta), Sin(theta) * Sin(phi));

            sphere.AddVertex({
                .position = pos,
                .normal = pos,
                .texCoord0 = { (float)segment / (float)numSegments, (float)ring / (float)numRings }
            });
        }
    }
    for (int ring = 1; ring <= numRings; ++ring)
    {
        for (int segment = 0; segment < numSegments; ++segment)
        {
            int start = ring * numSegments;
            int prev = start - numSegments;

            int pos = start + segment;

            sphere.AddIndex(pos);
            sphere.AddIndex(pos - numSegments);
            sphere.AddIndex(((pos - numSegments + 1) % numSegments) + prev);

            sphere.AddIndex(((pos - numSegments + 1) % numSegments) + prev);
            sphere.AddIndex(((pos + 1) % numSegments) + start);
            sphere.AddIndex(pos);
        }
    }
    return sphere;
}

void RenderSettings::InitializeDefaultResources(Device* device)
{
    // Create dummy textures
    uint32 black = 0xff000000;
    defaultBlackTexture = device->CreateTexture(TextureSettings{});
    defaultBlackTexture->Upload(sizeof(black), &black);

    uint32 white = 0xffffffff;
    defaultWhiteTexture = device->CreateTexture(TextureSettings{});
    defaultWhiteTexture->Upload(sizeof(white), &white);

    uint32 gray = 0xff808080;
    defaultGrayTexture = device->CreateTexture(TextureSettings{});
    defaultGrayTexture->Upload(sizeof(gray), &gray);

    uint32 normal = Color(0.5f, 0.5f, 1.0f).Pack();
    defaultNormalTexture = device->CreateTexture(TextureSettings{});
    defaultNormalTexture->Upload(sizeof(normal), &normal);

    uint32 green = 0xff00ff00;
    defaultGreenTexture = device->CreateTexture(TextureSettings{});
    defaultGreenTexture->Upload(sizeof(green), &green);

    cubeMesh = std::make_unique<StaticMesh>(device, CreateCube());
    quadMesh = std::make_unique<StaticMesh>(device, CreateQuad());
    sphereMesh = std::make_unique<StaticMesh>(device, CreateSphere());
}

}
