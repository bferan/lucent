#pragma once

#include <iostream>

#include "Vector4.hpp"
#include "Vector3.hpp"
#include "Math.hpp"
#include "Quaternion.hpp"

namespace lucent
{

//! 4x4 matrix of floats
struct Matrix4
{
public:
    static Matrix4 Diagonal(float s1, float s2, float s3, float s4)
    {
        return {
            Vector4(s1, 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, s2, 0.0f, 0.0f),
            Vector4(0.0f, 0.0f, s3, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, s4)
        };
    }

    static Matrix4 Identity()
    {
        return Diagonal(1.0f, 1.0f, 1.0f, 1.0f);
    }

    static Matrix4 Scale(float x, float y, float z)
    {
        return Diagonal(x, y, z, 1.0f);
    }

    static Matrix4 Translation(Vector3 by)
    {
        return {
            Vector4(1.0f, 0.0f, 0.f, 0.0f),
            Vector4(0.0f, 1.0f, 0.f, 0.0f),
            Vector4(0.0f, 0.0f, 1.f, 0.0f),
            Vector4(by.x, by.y, by.z, 1.0f)
        };
    }

    static Matrix4 RotationX(float angle)
    {
        auto c = Cos(angle);
        auto s = Sin(angle);
        return {
            Vector4(1.0f, 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, c, s, 0.0f),
            Vector4(0.0f, -s, c, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f)
        };
    }

    static Matrix4 RotationY(float angle)
    {
        auto c = Cos(angle);
        auto s = Sin(angle);
        return {
            Vector4(c, 0.0f, -s, 0.0f),
            Vector4(0.0f, 1.0f, 0.0f, 0.0f),
            Vector4(s, 0.0f, c, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f)
        };
    }

    static Matrix4 RotationZ(float angle)
    {
        auto c = Cos(angle);
        auto s = Sin(angle);
        return {
            Vector4(c, s, 0.0f, 0.0f),
            Vector4(-s, c, 0.0f, 0.0f),
            Vector4(0.0f, 0.0f, 1.0f, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f)
        };
    }

    static Matrix4 Rotation(Quaternion q)
    {
        auto x2 = q.x * q.x;
        auto y2 = q.y * q.y;
        auto z2 = q.z * q.z;
        auto xy = q.x * q.y;
        auto xz = q.x * q.z;
        auto yz = q.y * q.z;
        auto wx = q.w * q.x;
        auto wy = q.w * q.y;
        auto wz = q.w * q.z;

        return {
            Vector4(1.0f - 2.0f * (y2 + z2), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f),
            Vector4(2.0f * (xy - wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz + wx), 0.0f),
            Vector4(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (x2 + y2), 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f)
        };
    }

    static Quaternion Rotation(const Matrix4& m)
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
        Quaternion q;

        auto m00 = m(0, 0);
        auto m11 = m(1, 1);
        auto m22 = m(2, 2);
        auto trace = m00 + m11 + m22;

        if (trace > 0.0f)
        {
            q.w = Sqrt(trace + 1.0f) * 0.5f;
            auto s = 0.25f / q.w;
            q.x = (m(2, 1) - m(1, 2)) * s;
            q.y = (m(0, 2) - m(2, 0)) * s;
            q.z = (m(1, 0) - m(0, 1)) * s;
        }
        else if ((m00 > m11) && (m00 > m22))
        {
            q.x = Sqrt(1.0f + m00 - m11 - m22) * 0.5f;
            auto s = 0.25f / q.x;
            q.w = (m(2, 1) - m(1, 2)) * s;
            q.y = (m(0, 1) + m(1, 0)) * s;
            q.z = (m(0, 2) + m(2, 0)) * s;
        }
        else if (m11 > m22)
        {
            q.y = Sqrt(1.0f + m11 - m00 - m22) * 0.5f;
            auto s = 0.25f / q.y;
            q.w = (m(0, 2) - m(2, 0)) * s;
            q.x = (m(0, 1) + m(1, 0)) * s;
            q.z = (m(1, 2) + m(2, 1)) * s;
        }
        else
        {
            q.z = Sqrt(1.0f + m22 - m00 - m11) * 0.5f;
            auto s = 0.25f / q.z;
            q.w = (m(1, 0) - m(0, 1)) * s;
            q.x = (m(0, 2) + m(2, 0)) * s;
            q.y = (m(1, 2) + m(2, 1)) * s;
        }
        return q;
    }

    static Matrix4 Perspective(float verticalFov, float aspectRatio, float n, float f)
    {
        // Compute focal distance from fov
        auto focalLength = 1.0f / Tan(verticalFov / 2.0f);
        auto k = f / (f - n);

        return {
            Vector4(focalLength / aspectRatio, 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, focalLength, 0.0f, 0.0f),
            Vector4(0.0f, 0.0f, k, 1.0f),
            Vector4(0.0f, 0.0f, -n * k, 0.0f)
        };
    }

    static Matrix4 Orthographic(float w, float h, float d)
    {
        return Diagonal(2.0f / w, 2.0f / h, 1.0f / d, 1.0f);
    }

    static Matrix4 LookAt(Vector3 fromPos, Vector3 toPos, Vector3 up = Vector3::Up())
    {
        auto back = fromPos - toPos;
        back.Normalize();

        if (Approximately(back, up))
            up = Vector3::Forward();

        auto right = up.Cross(back);
        right.Normalize();

        up = back.Cross(right);

        return {
            Vector4{ right, 0.0f },
            Vector4{ up, 0.0f },
            Vector4{ back, 0.0f },
            Vector4{}
        };
    }

public:
    Matrix4(const Matrix4&) = default;
    Matrix4(Matrix4&&) = default;

    Matrix4() : Matrix4(Matrix4::Identity())
    {}

    Matrix4(Vector4 c1, Vector4 c2, Vector4 c3, Vector4 c4)
        : c1(c1), c2(c2), c3(c3), c4(c4)
    {}

    explicit Matrix4(Quaternion q)
        : Matrix4(Matrix4::Rotation(q))
    {}

    Matrix4& operator=(const Matrix4&) = default;
    Matrix4& operator=(Matrix4&&) = default;

    const Vector4& operator[](int index) const
    {
        return (&c1)[index];
    }
    Vector4& operator[](int index)
    {
        return (&c1)[index];
    }

    float operator()(int row, int col) const
    {
        return (*this)[col][row];
    }

    float& operator()(int row, int col)
    {
        return (*this)[col][row];
    }

    Matrix4 Transposed() const
    {
        return {
            Vector4(c1.x, c2.x, c3.x, c4.x),
            Vector4(c1.y, c2.y, c3.y, c4.y),
            Vector4(c1.z, c2.z, c3.z, c4.z),
            Vector4(c1.w, c2.w, c3.w, c4.w) };
    }

    void Decompose(Vector3& pos, Quaternion& rot, Vector3& scale) const
    {
        auto m = *this;

        // Translation
        pos = Vector3(m.c4);
        m.c4 = Vector4();

        // Scale
        scale.x = Vector3(m.c1).Length();
        scale.y = Vector3(m.c2).Length();
        scale.z = Vector3(m.c3).Length();
        m.c1 /= scale.x;
        m.c2 /= scale.y;
        m.c3 /= scale.z;

        // Rotation
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

        rot.w = Sqrt(Max(0.0f, 1.0f + m[0][0] + m[1][1] + m[2][2])) / 2.0f;
        rot.x = Sqrt(Max(0.0f, 1.0f + m[0][0] - m[1][1] - m[2][2])) / 2.0f;
        rot.y = Sqrt(Max(0.0f, 1.0f - m[0][0] + m[1][1] - m[2][2])) / 2.0f;
        rot.z = Sqrt(Max(0.0f, 1.0f - m[0][0] - m[1][1] + m[2][2])) / 2.0f;

        rot.x = CopySign(rot.x, m[1][2] - m[2][1]);
        rot.y = CopySign(rot.y, m[2][0] - m[0][2]);
        rot.z = CopySign(rot.z, m[0][1] - m[1][0]);
    }

    Vector4 Row(int row)
    {
        auto& m = *this;
        return { m(row, 0), m(row, 1), m(row, 2), m(row, 3) };
    }

public:
    Vector4 c1;
    Vector4 c2;
    Vector4 c3;
    Vector4 c4;
};

inline Matrix4 operator*(const Matrix4& l, const Matrix4& r)
{
    return {
        Vector4(
            l[0][0] * r[0][0] + l[1][0] * r[0][1] + l[2][0] * r[0][2] + l[3][0] * r[0][3],
            l[0][1] * r[0][0] + l[1][1] * r[0][1] + l[2][1] * r[0][2] + l[3][1] * r[0][3],
            l[0][2] * r[0][0] + l[1][2] * r[0][1] + l[2][2] * r[0][2] + l[3][2] * r[0][3],
            l[0][3] * r[0][0] + l[1][3] * r[0][1] + l[2][3] * r[0][2] + l[3][3] * r[0][3]),
        Vector4(
            l[0][0] * r[1][0] + l[1][0] * r[1][1] + l[2][0] * r[1][2] + l[3][0] * r[1][3],
            l[0][1] * r[1][0] + l[1][1] * r[1][1] + l[2][1] * r[1][2] + l[3][1] * r[1][3],
            l[0][2] * r[1][0] + l[1][2] * r[1][1] + l[2][2] * r[1][2] + l[3][2] * r[1][3],
            l[0][3] * r[1][0] + l[1][3] * r[1][1] + l[2][3] * r[1][2] + l[3][3] * r[1][3]),
        Vector4(
            l[0][0] * r[2][0] + l[1][0] * r[2][1] + l[2][0] * r[2][2] + l[3][0] * r[2][3],
            l[0][1] * r[2][0] + l[1][1] * r[2][1] + l[2][1] * r[2][2] + l[3][1] * r[2][3],
            l[0][2] * r[2][0] + l[1][2] * r[2][1] + l[2][2] * r[2][2] + l[3][2] * r[2][3],
            l[0][3] * r[2][0] + l[1][3] * r[2][1] + l[2][3] * r[2][2] + l[3][3] * r[2][3]),
        Vector4(
            l[0][0] * r[3][0] + l[1][0] * r[3][1] + l[2][0] * r[3][2] + l[3][0] * r[3][3],
            l[0][1] * r[3][0] + l[1][1] * r[3][1] + l[2][1] * r[3][2] + l[3][1] * r[3][3],
            l[0][2] * r[3][0] + l[1][2] * r[3][1] + l[2][2] * r[3][2] + l[3][2] * r[3][3],
            l[0][3] * r[3][0] + l[1][3] * r[3][1] + l[2][3] * r[3][2] + l[3][3] * r[3][3]) };
}

inline Vector4 operator*(const Matrix4& l, const Vector4& r)
{
    return {
        l[0][0] * r[0] + l[1][0] * r[1] + l[2][0] * r[2] + l[3][0] * r[3],
        l[0][1] * r[0] + l[1][1] * r[1] + l[2][1] * r[2] + l[3][1] * r[3],
        l[0][2] * r[0] + l[1][2] * r[1] + l[2][2] * r[2] + l[3][2] * r[3],
        l[0][3] * r[0] + l[1][3] * r[1] + l[2][3] * r[2] + l[3][3] * r[3] };
}

}

