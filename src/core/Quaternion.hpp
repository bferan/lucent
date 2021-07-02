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
        return { s * axis.x, s * axis.y, s * axis.z, c };
    }

public:
    Quaternion(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f)
        : x(x), y(y), z(z), w(w)
    {}

    Quaternion Inverse() const
    {
        return { -x, -y, -z, w };
    }

public:
    float x;
    float y;
    float z;
    float w;
};

inline Quaternion operator*(Quaternion q, Quaternion r)
{
    return
        {
            q.y * r.z - q.z * r.y + r.w * q.x + q.w * r.x,
            q.z * r.x - q.x * r.z + r.w * q.y + q.w * r.y,
            q.x * r.y - q.y * r.x + r.w * q.z + q.w * r.z,
            q.w * r.w - q.x * r.x - q.y * r.y - q.z * r.z
        };
}

}