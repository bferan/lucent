#pragma once

#include "Math.hpp"

namespace lucent
{

struct Vector2
{
public:
    inline static Vector2 Up()
    {
        return { 0.0f, 1.0f };
    }
    inline static Vector2 Down()
    {
        return { 0.0f, -1.0f };
    }
    inline static Vector2 Right()
    {
        return { 1.0f, 0.0f };
    }
    inline static Vector2 Left()
    {
        return { -1.0f, 0.0f };
    }

    inline static Vector2 One()
    {
        return { 1.0f, 1.0f };
    }
    inline static Vector2 Zero()
    {
        return { 0.0f, 0.0f };
    }

public:
    Vector2(float x = 0.0f, float y = 0.0f)
        : x(x), y(y)
    {}

    Vector2(const Vector2&) = default;
    Vector2(Vector2&&) = default;

    Vector2& operator=(const Vector2&) = default;
    Vector2& operator=(Vector2&&) = default;

    float operator[](int index) const
    {
        return (&x)[index];
    }
    float& operator[](int index)
    {
        return (&x)[index];
    }

    float Length() const
    {
        return Sqrt(x * x + y * y);
    }

    Vector2& operator+=(const Vector2& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vector2& operator*=(const Vector2& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    Vector2& operator*=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }

public:
    float x;
    float y;
};

inline Vector2 operator*(Vector2 lhs, float rhs)
{
    return (lhs *= rhs);
}

inline Vector2 operator*(float lhs, Vector2 rhs)
{
    return (rhs *= lhs);
}

inline Vector2 operator*(Vector2 lhs, const Vector2& rhs)
{
    return (lhs *= rhs);
}

inline Vector2 operator-(const Vector2& rhs)
{
    return { -rhs.x, -rhs.y };
}

inline Vector2 operator+(Vector2 lhs, const Vector2& rhs)
{
    return (lhs += rhs);
}

inline Vector2 operator-(Vector2 lhs, const Vector2& rhs)
{
    return (lhs -= rhs);
}

}
