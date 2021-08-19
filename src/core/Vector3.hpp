#pragma once

#include "Math.hpp"

namespace lucent
{

struct Vector3
{
public:
    inline static Vector3 Up()
    {
        return { 0.0f, 1.0f, 0.0f };
    }
    inline static Vector3 Down()
    {
        return { 0.0f, -1.0f, 0.0f };
    }
    inline static Vector3 Right()
    {
        return { 1.0f, 0.0f, 0.0f };
    }
    inline static Vector3 Left()
    {
        return { -1.0f, 0.0f, 0.0f };
    }
    inline static Vector3 Forward()
    {
        return { 0.0f, 0.0f, -1.0f };
    }
    inline static Vector3 Back()
    {
        return { 0.0f, 0.0f, 1.0f };
    }
    inline static Vector3 One()
    {
        return { 1.0f, 1.0f, 1.0f };
    }
    inline static Vector3 Zero()
    {
        return { 0.0f, 0.0f, 0.0f };
    }
    inline static Vector3 Infinity()
    {
        return { HUGE_VALF, HUGE_VALF, HUGE_VALF };
    }
    inline static Vector3 NegativeInfinity()
    {
        return { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
    }

public:
    Vector3()
        : Vector3(0.0f, 0.0f, 0.0f)
    {}

    Vector3(float x, float y, float z = 0.0f)
        : x(x), y(y), z(z)
    {}

    Vector3(const Vector3&) = default;
    Vector3(Vector3&&) = default;

    Vector3& operator=(const Vector3&) = default;
    Vector3& operator=(Vector3&&) = default;

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
        return Sqrt(x * x + y * y + z * z);
    }

    void Normalize()
    {
        auto length = Length();
        if (!Approximately(length, 0.0f))
        {
            (*this) *= 1.0f / Length();
        }
    }

    Vector3 Cross(Vector3 rhs) const
    {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    float Dot(Vector3 rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    Vector3& operator+=(const Vector3& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    Vector3& operator*=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

public:
    float x;
    float y;
    float z;
};

inline Vector3 operator*(Vector3 lhs, float rhs)
{
    return (lhs *= rhs);
}

inline Vector3 operator*(float lhs, Vector3 rhs)
{
    return (rhs *= lhs);
}

inline Vector3 operator-(const Vector3& rhs)
{
    return { -rhs.x, -rhs.y, -rhs.z };
}

inline Vector3 operator+(Vector3 lhs, const Vector3& rhs)
{
    return (lhs += rhs);
}

inline Vector3 operator-(Vector3 lhs, const Vector3& rhs)
{
    return (lhs -= rhs);
}

inline Vector3 Min(Vector3 lhs, Vector3 rhs)
{
    return { Min(lhs.x, rhs.x), Min(lhs.y, rhs.y), Min(lhs.z, rhs.z) };
}

inline Vector3 Max(Vector3 lhs, Vector3 rhs)
{
    return { Max(lhs.x, rhs.x), Max(lhs.y, rhs.y), Max(lhs.z, rhs.z) };
}

inline bool Approximately(Vector3 lhs, Vector3 rhs)
{
    return Approximately(lhs.x, rhs.x) &&
        Approximately(lhs.y, rhs.y) &&
        Approximately(lhs.z, rhs.z);
}

}

