#pragma once

#include "debug/DebugConsole.hpp"
#include "rendering/Renderer.hpp"

namespace lucent
{

constexpr uint32 kMaxDebugShapes = 1024;

struct alignas(sizeof(Vector4)) DebugShape
{
    Vector4 color;
    Vector3 srcPos;
    float radius;
    Vector3 dstPos;
    uint32 type;
};

struct DebugShapeBuffer
{
    uint32 numShapes;
    DebugShape shapes[kMaxDebugShapes];
};

void AddDebugOverlayPass(Renderer& renderer, DebugConsole* console, Texture* output);

}
