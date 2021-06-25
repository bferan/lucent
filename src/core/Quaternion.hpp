#pragma once

#include "Vector3.hpp"
#include "Math.hpp"
#include "Vector4.hpp"

namespace lucent
{

struct Quaternion
{
public:
    static Quaternion AxisAngle(Vector3 axis, float angle)
    {
        auto c = Cos(angle * 0.5f);
        auto s = Sin(angle * 0.5f);
        return { s * axis.X, s * axis.Y, s * axis.Z, c };
    }

public:
    Quaternion(float x, float y, float z, float w)
        : X(x), Y(y), Z(z), W(w)
    {}

    Quaternion Inverse() const
    {
        return { -X, -Y, -Z, W };
    }

public:
    float X;
    float Y;
    float Z;
    float W;
};

inline Quaternion operator*(Quaternion q, Quaternion r)
{
    return
        {
            q.Y * r.Z - q.Z * r.Y + r.W * q.X + q.W * r.X,
            q.Z * r.X - q.X * r.Z + r.W * q.Y + q.W * r.Y,
            q.X * r.Y - q.Y * r.X + r.W * q.Z + q.W * r.Z,
            q.W * r.W - q.X * r.X - q.Y * r.Y - q.Z * r.Z
        };
}

}