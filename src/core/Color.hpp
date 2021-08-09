#pragma once

#include "core/Vector4.hpp"
#include "core/Math.hpp"

namespace lucent
{

using PackedColor = uint32;

struct Color
{
public:
    static Color White()
    {
        return { 1.0f, 1.0f, 1.0f };
    }
    static Color Gray()
    {
        return { 0.5f, 0.5f, 0.5f };
    }
    static Color Black()
    {
        return { 0.0f, 0.0f, 0.0f };
    }
    static Color Red()
    {
        return { 1.0f, 0.0f, 0.0f };
    }
    static Color Green()
    {
        return { 0.0f, 1.0f, 0.0f };
    }
    static Color Blue()
    {
        return { 0.0f, 0.0f, 1.0f };
    }

public:
    Color(float r, float g, float b, float a = 1.0f)
        : r(r), g(g), b(b), a(a)
    {}

    explicit Color(Vector4 v)
        : r(v.x), g(v.y), b(v.z), a(v.w)
    {}

    Color(const Color&) = default;
    Color(Color&&) = default;
    Color& operator=(const Color&) = default;
    Color& operator=(Color&&) = default;

    explicit operator Vector4() const
    {
        return { r, g, b, a };
    }

    PackedColor Pack() const
    {
        return PackedColor(r * 255) |
            PackedColor(g * 255) << 8 |
            PackedColor(b * 255) << 16 |
            PackedColor(a * 255) << 24;
    }

public:
    float r;
    float g;
    float b;
    float a;
};

}