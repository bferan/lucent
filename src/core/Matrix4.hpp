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
            Vector4(by.X, by.Y, by.Z, 1.0f));
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
            Vector4(1.0f - 2 * (q.Y * q.Y + q.Z * q.Z), 2 * (q.X * q.Y + q.Z * q.W), 2 * (q.X * q.Z - q.Y * q.W), 0.0f),
            Vector4(2 * (q.X * q.Y - q.Z * q.W), 1.0f - 2 * (q.X * q.X + q.Z * q.Z), 2 * (q.Y * q.Z + q.X * q.W), 0.0f),
            Vector4(2 * (q.X * q.Z + q.Y * q.W), 2 * (q.Y * q.Z - q.X * q.W), 1.0f - 2 * (q.X * q.X + q.Y * q.Y), 0.0f),
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

        //return Perspective(-n / e, n / e, aspectRatio * n / e, -aspectRatio * n / e, n, f);
    }

public:
    Matrix4(const Matrix4&) = default;
    Matrix4(Matrix4&&) = default;

    Matrix4() : Matrix4(Matrix4::Identity())
    {}

    Matrix4(Vector4 c1, Vector4 c2, Vector4 c3, Vector4 c4)
        : C1(c1), C2(c2), C3(c3), C4(c4)
    {}

    Matrix4& operator=(const Matrix4&) = default;
    Matrix4& operator=(Matrix4&&) = default;

    const Vector4& operator[](int index) const
    {
        return (&C1)[index];
    }
    Vector4& operator[](int index)
    {
        return (&C1)[index];
    }

    Matrix4 Transpose()
    {
        return Matrix4(
            Vector4(C1.X, C2.X, C3.X, C4.X),
            Vector4(C1.Y, C2.Y, C3.Y, C4.Y),
            Vector4(C1.Z, C2.Z, C3.Z, C4.Z),
            Vector4(C1.W, C2.W, C3.W, C4.W));
    }

    void Decompose(Vector3& pos, Quaternion& rot, Vector3& scale) const
    {
        auto m = *this;

        // Translation
        pos = Vector3(m.C4);
        m.C4 = Vector4();

        // Scale
        scale.X = Vector3(m.C1).Length();
        scale.Y = Vector3(m.C2).Length();
        scale.Z = Vector3(m.C3).Length();
        m.C1 /= scale.X;
        m.C2 /= scale.Y;
        m.C3 /= scale.Z;

        // Rotation
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

        rot.W = Sqrt(Max(0.0f, 1.0f + m[0][0] + m[1][1] + m[2][2])) / 2.0f;
        rot.X = Sqrt(Max(0.0f, 1.0f + m[0][0] - m[1][1] - m[2][2])) / 2.0f;
        rot.Y = Sqrt(Max(0.0f, 1.0f - m[0][0] + m[1][1] - m[2][2])) / 2.0f;
        rot.Z = Sqrt(Max(0.0f, 1.0f - m[0][0] - m[1][1] + m[2][2])) / 2.0f;

        rot.X = CopySign(rot.X, m[1][2] - m[2][1]);
        rot.Y = CopySign(rot.Y, m[2][0] - m[0][2]);
        rot.Z = CopySign(rot.Z, m[0][1] - m[1][0]);
    }

public:
    Vector4 C1;
    Vector4 C2;
    Vector4 C3;
    Vector4 C4;
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

inline std::ostream& operator<<(std::ostream& out, const Matrix4& mat)
{
    out << "|" << mat[0][0] << ", " << mat[1][0] << ", " << mat[2][0] << ", " << mat[3][0] << "|\n";
    out << "|" << mat[0][1] << ", " << mat[1][1] << ", " << mat[2][1] << ", " << mat[3][1] << "|\n";
    out << "|" << mat[0][2] << ", " << mat[1][2] << ", " << mat[2][2] << ", " << mat[3][2] << "|\n";
    out << "|" << mat[0][3] << ", " << mat[1][3] << ", " << mat[2][3] << ", " << mat[3][3] << "|\n";
    return out;
}
}

