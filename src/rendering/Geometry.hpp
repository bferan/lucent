#pragma once

#include "device/Device.hpp"

namespace lucent
{

// TODO: REMOVE
inline struct Cube
{
    Buffer* vertices;
    Buffer* indices;
    uint32 numIndices;
} g_Cube;

inline struct Quad
{
    Buffer* vertices;
    Buffer* indices;
    uint32 numIndices;
} g_Quad;

inline struct Sphere
{
    Buffer* vertices;
    Buffer* indices;
    uint32 numIndices;
} g_Sphere;

inline Texture* g_BlackTexture;
inline Texture* g_WhiteTexture;
inline Texture* g_GrayTexture;
inline Texture* g_GreenTexture;
inline Texture* g_NormalTexture;

void InitGeometry(Device* device);

}
