#pragma once

#include "core/Vector4.hpp"
#include "core/Math.hpp"

namespace lucent
{

struct Color
{
public:
    static Color White() { return Color(1.0f, 1.0f, 1.0f); }
    static Color Gray() { return Color(0.5f, 0.5f, 0.5f); }
    static Color Black() { return Color(0.0f, 0.0f, 0.0f); }
    static Color Red() { return Color(1.0f, 0.0f, 0.0f); }
    static Color Green() { return Color(0.0f, 1.0f, 0.0f); }
    static Color Blue() { return Color(0.0f, 0.0f, 1.0f); }

public:
    Color(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f)
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
        return Vector4(r, g, b, a);
    }

public:
    float r;
    float g;
    float b;
    float a;
};

}