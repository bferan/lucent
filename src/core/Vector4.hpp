#pragma once

#include <iostream>

#include "Vector3.hpp"

namespace lucent
{

struct Vector4
{
public:
    explicit Vector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f)
        : X(x), Y(y), Z(z), W(w)
    {}

    explicit Vector4(const Vector3& v, float w = 1.0f)
        : X(v.X), Y(v.Y), Z(v.Z), W(w)
    {}

    explicit operator Vector3() const
    {
        return Vector3(X, Y, Z);
    }

    Vector4(const Vector4&) = default;
    Vector4(Vector4&&) = default;

    Vector4& operator=(const Vector4&) = default;
    Vector4& operator=(Vector4&&) = default;

    float operator[](int index) const { return (&X)[index]; }

    float& operator[](int index) { return (&X)[index]; }

    Vector4& operator+=(const Vector4& rhs)
    {
        X += rhs.X;
        Y += rhs.Y;
        Z += rhs.Z;
        W += rhs.W;
        return *this;
    }

    Vector4& operator-=(const Vector4& rhs)
    {
        X -= rhs.X;
        Y -= rhs.Y;
        Z -= rhs.Z;
        W -= rhs.W;
        return *this;
    }

    Vector4& operator*=(float rhs)
    {
        X *= rhs;
        Y *= rhs;
        Z *= rhs;
        W *= rhs;
        return *this;
    }

    Vector4& operator/=(float rhs)
    {
        X /= rhs;
        Y /= rhs;
        Z /= rhs;
        W /= rhs;
        return *this;
    }

public:
    float X;
    float Y;
    float Z;
    float W;
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
    return Vector4(-rhs.X, -rhs.Y, -rhs.Z, -rhs.W);
}

inline Vector4 operator+(Vector4 lhs, const Vector4& rhs)
{
    return (lhs += rhs);
}

inline Vector4 operator-(Vector4 lhs, const Vector4& rhs)
{
    return (lhs -= rhs);
}

// TODO: Remove
inline std::ostream& operator<<(std::ostream& out, const Vector4& v)
{
    out << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
    return out;
}

}
