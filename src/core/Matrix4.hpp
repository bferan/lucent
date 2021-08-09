#pragma once

#include <iostream>

#include "Vector4.hpp"
#include "Vector3.hpp"
#include "Math.hpp"
#include "Quaternion.hpp"

namespace lucent
{

struct Matrix4
{
public:
    static Matrix4 Diagonal(float s1, float s2, float s3, float s4)
    {
        return Matrix4(
            Vector4(s1, 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, s2, 0.0f, 0.0f),
            Vector4(0.0f, 0.0f, s3, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, s4));
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
        return Matrix4(
            Vector4(1.0f, 0.0f, 0.f, 0.0f),
            Vector4(0.0f, 1.0f, 0.f, 0.0f),
            Vector4(0.0f, 0.0f, 1.f, 0.0f),
            Vector4(by.x, by.y, by.z, 1.0f));
    }

    static Matrix4 RotationX(float angle)
    {
        auto c = Cos(angle);
        auto s = Sin(angle);
        return Matrix4(
            Vector4(1.0f, 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, c, s, 0.0f),
            Vector4(0.0f, -s, c, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    static Matrix4 RotationY(float angle)
    {
        auto c = Cos(angle);
        auto s = Sin(angle);
        return Matrix4(
            Vector4(c, 0.0f, -s, 0.0f),
            Vector4(0.0f, 1.0f, 0.0f, 0.0f),
            Vector4(s, 0.0f, c, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    static Matrix4 RotationZ(float angle)
    {
        auto c = Cos(angle);
        auto s = Sin(angle);
        return Matrix4(
            Vector4(c, s, 0.0f, 0.0f),
            Vector4(-s, c, 0.0f, 0.0f),
            Vector4(0.0f, 0.0f, 1.0f, 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    static Matrix4 Rotation(Quaternion q)
    {
        return Matrix4(
            Vector4(1.0f - 2 * (q.y * q.y + q.z * q.z), 2 * (q.x * q.y + q.z * q.w), 2 * (q.x * q.z - q.y * q.w), 0.0f),
            Vector4(2 * (q.x * q.y - q.z * q.w), 1.0f - 2 * (q.x * q.x + q.z * q.z), 2 * (q.y * q.z + q.x * q.w), 0.0f),
            Vector4(2 * (q.x * q.z + q.y * q.w), 2 * (q.y * q.z - q.x * q.w), 1.0f - 2 * (q.x * q.x + q.y * q.y), 0.0f),
            Vector4(0.0f, 0.0f, 0.0f, 1.0f)
        );
    }

    static Matrix4 Perspective(float l, float r, float t, float b, float n, float f)
    {
        return Matrix4(
            Vector4((2.0f * n) / (r - l), 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, (2.0f * n) / (t - b), 0.0f, 0.0f),
            Vector4((r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1.0f),
            Vector4(0.0f, 0.0f, -(2 * n * f) / (f - n), 0.0f));
    }

    static Matrix4 Perspective(float horizontalFov, float aspectRatio, float n, float f)
    {
        // Compute focal distance 'e' from fov
        auto e = 1.0f / Tan(horizontalFov / 2.0f);

        return Matrix4(
            Vector4(e / aspectRatio, 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, e, 0.0f, 0.0f),
            Vector4(0.0f, 0.0f, -(f + n) / (f - n), -1.0f),
            Vector4(0.0f, 0.0f, -(2.0f * f * n) / (f - n), 0.0f)
        );

        //return Perspective(-b / e, b / e, aspectRatio * b / e, -aspectRatio * b / e, b, f);
    }

public:
    Matrix4(const Matrix4&) = default;
    Matrix4(Matrix4&&) = default;

    Matrix4() : Matrix4(Matrix4::Identity())
    {}

    Matrix4(Vector4 c1, Vector4 c2, Vector4 c3, Vector4 c4)
        : c1(c1), c2(c2), c3(c3), c4(c4)
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

    Matrix4 Transpose()
    {
        return Matrix4(
            Vector4(c1.x, c2.x, c3.x, c4.x),
            Vector4(c1.y, c2.y, c3.y, c4.y),
            Vector4(c1.z, c2.z, c3.z, c4.z),
            Vector4(c1.w, c2.w, c3.w, c4.w));
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

public:
    Vector4 c1;
    Vector4 c2;
    Vector4 c3;
    Vector4 c4;
};

inline Matrix4 operator*(const Matrix4& l, const Matrix4& r)
{
    return Matrix4(
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
            l[0][3] * r[3][0] + l[1][3] * r[3][1] + l[2][3] * r[3][2] + l[3][3] * r[3][3]));
}

inline Vector4 operator*(const Matrix4& l, const Vector4& r)
{
    return Vector4(
        l[0][0] * r[0] + l[1][0] * r[1] + l[2][0] * r[2] + l[3][0] * r[3],
        l[0][1] * r[0] + l[1][1] * r[1] + l[2][1] * r[2] + l[3][1] * r[3],
        l[0][2] * r[0] + l[1][2] * r[1] + l[2][2] * r[2] + l[3][2] * r[3],
        l[0][3] * r[0] + l[1][3] * r[1] + l[2][3] * r[2] + l[3][3] * r[3]);
}

}

