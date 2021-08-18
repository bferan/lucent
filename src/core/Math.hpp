#pragma once

#include <cmath>
#include "math.h"

namespace lucent
{

constexpr float kPi = 3.14159265358979323846;
constexpr float k2Pi = kPi * 2.0f;
constexpr float kHalfPi = kPi / 2.0f;

inline float Sqrt(float x)
{
    return std::sqrt(x);
}

inline float Sin(float radians)
{
    return std::sin(radians);
}

inline float Asin(float x)
{
    return std::asin(x);
}

inline float Cos(float radians)
{
    return std::cos(radians);
}

inline float Acos(float x)
{
    return std::acos(x);
}

inline float Tan(float radians)
{
    return std::tan(radians);
}

inline float Atan(float x)
{
    return std::atan(x);
}

inline float Atan2(float x, float y)
{
    return std::atan2(x, y);
}

inline float Clamp(float value, float min, float max)
{
    return value > min ? (value < max ? value : max) : min;
}

inline float Ceil(float value)
{
    return std::ceil(value);
}

inline float Floor(float value)
{
    return std::floor(value);
}

inline float Round(float value)
{
    return Floor(value + 0.5f);
}

inline float Mod(float value, float by)
{
    return std::fmod(value, by);
}

inline float Pow(float base, float exp)
{
    return std::powf(base, exp);
}

inline float Abs(float x)
{
    return std::abs(x);
}

inline float CopySign(float value, float sign)
{
    return std::copysign(value, sign);
}

inline bool Approximately(float x, float y, float epsilon = FLT_EPSILON)
{
    return Abs(x - y) <= epsilon;
}

template<typename T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
T Max(T a, T b)
{
    return a < b ? b : a;
}

}