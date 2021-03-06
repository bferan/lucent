#pragma once

#include "Vector3.hpp"

namespace lucent
{

struct Vector4
{
public:
    Vector4()
        : Vector4(0.0f, 0.0f, 0.0f, 1.0f)
    {}

    Vector4(float x, float y, float z, float w = 1.0f)
        : x(x), y(y), z(z), w(w)
    {}

    explicit Vector4(const Vector3& v, float w = 1.0f)
        : x(v.x), y(v.y), z(v.z), w(w)
    {}

    explicit operator Vector3() const
    {
        return { x, y, z };
    }

    Vector4(const Vector4&) = default;
    Vector4(Vector4&&) = default;

    Vector4& operator=(const Vector4&) = default;
    Vector4& operator=(Vector4&&) = default;

    float operator[](int index) const
    {
        return (&x)[index];
    }

    float& operator[](int index)
    {
        return (&x)[index];
    }

    Vector4& operator+=(const Vector4& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    Vector4& operator-=(const Vector4& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    Vector4& operator*=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
        return *this;
    }

    Vector4& operator/=(float rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        w /= rhs;
        return *this;
    }

public:
    float x;
    float y;
    float z;
    float w;
};

inline Vector4 operator*(Vector4 lhs, float rhs)
{
    return (lhs *= rhs);
}

inline Vector4 operator*(float lhs, Vector4 rhs)
{
    return (rhs *= lhs);
}

// Negation
inline Vector4 operator-(const Vector4& rhs)
{
    return { -rhs.x, -rhs.y, -rhs.z, -rhs.w };
}

inline Vector4 operator+(Vector4 lhs, const Vector4& rhs)
{
    return (lhs += rhs);
}

inline Vector4 operator-(Vector4 lhs, const Vector4& rhs)
{
    return (lhs -= rhs);
}
}
