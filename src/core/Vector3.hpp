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
        if (length != 0.0)
        {
            (*this) *= 1.0f / Length();
        }
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

}

