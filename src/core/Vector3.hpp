#pragma once

#include "Math.hpp"

namespace lucent
{

struct Vector3
{
public:
    inline static Vector3 Up() { return Vector3(0.0f, 1.0f, 0.0f); }
    inline static Vector3 Down() { return Vector3(0.0f, -1.0f, 0.0f); }
    inline static Vector3 Right() { return Vector3(1.0f, 0.0f, 0.0f); }
    inline static Vector3 Left() { return Vector3(-1.0f, 0.0f, 0.0f); }
    inline static Vector3 Forward() { return Vector3(0.0f, 0.0f, -1.0f); }
    inline static Vector3 Back() { return Vector3(0.0f, 0.0f, 1.0f); }

public:
    explicit Vector3(float x = 0.0f, float y = 0.0f, float z = 0.0f)
        : X(x), Y(y), Z(z)
    {}
    Vector3(const Vector3&) = default;
    Vector3(Vector3&&) = default;

    Vector3& operator=(const Vector3&) = default;
    Vector3& operator=(Vector3&&) = default;

    float operator[](int index) const { return (&X)[index]; }
    float& operator[](int index) { return (&X)[index]; }

    float Length() const
    {
        return Sqrt(X * X + Y * Y + Z * Z);
    }

    Vector3& operator+=(const Vector3& rhs)
    {
        X += rhs.X;
        Y += rhs.Y;
        Z += rhs.Z;
        return *this;
    }

    Vector3& operator-=(const Vector3& rhs)
    {
        X -= rhs.X;
        Y -= rhs.Y;
        Z -= rhs.Z;
        return *this;
    }

    Vector3& operator*=(float rhs)
    {
        X *= rhs;
        Y *= rhs;
        Z *= rhs;
        return *this;
    }


public:
    float X;
    float Y;
    float Z;
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
    return Vector3(-rhs.X, -rhs.Y, -rhs.Z);
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
