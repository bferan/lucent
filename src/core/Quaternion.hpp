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

    static Quaternion FromTo(Vector3 fromDirection, Vector3 toDirection, Vector3 up = Vector3::Up())
    {
        fromDirection.Normalize();
        toDirection.Normalize();

        auto dot = fromDirection.Dot(toDirection);

        if (Approximately(dot, -1.0f))
        {
            return Quaternion::AxisAngle(up, kPi);
        }
        else if (Approximately(dot, 1.0f))
        {
            return {};
        }

        auto axis = fromDirection.Cross(toDirection);
        auto angle = Acos(dot);
        return Quaternion::AxisAngle(axis, angle);
    }

public:
    Quaternion()
        : Quaternion(0.0f, 0.0f, 0.0f, 1.0f)
    {}

    Quaternion(float x, float y, float z, float w)
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