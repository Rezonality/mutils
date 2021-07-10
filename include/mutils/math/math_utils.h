#pragma once

#include <mutils/math/math.h>
#include <algorithm>

namespace MUtils
{
float SmoothStep(float val);

template <class T>
bool IsRectEmpty(const T& rect)
{
    return (rect.z == 0 || rect.w == 0);
}

template <typename T, typename P>
bool RectContains(const T& rect, const P& point)
{
    return ((rect.x <= point.x && (rect.x + rect.z) >= point.x) && (rect.y <= point.y && (rect.y + rect.w) >= point.y));
}

double RandRange(double begin, double end);
float RandRange(float begin, float end);
uint32_t RandRange(uint32_t min, uint32_t max);
int32_t RandRange(int32_t begin, int32_t end);

template<class T>
void GetBounds(const NVec3<T>* coords, uint32_t count, NVec3<T>& min, NVec3<T>& max)
{
    if (count == 0 || coords == nullptr)
    {
        min = NVec3<T>();
        max = NVec3<T>();
        return;
    }

    min = NVec3<T>(std::numeric_limits<float>::max());
    max = NVec3<T>(-std::numeric_limits<float>::max());
    for (uint32_t i = 0; i < count; i++)
    {
        min = Min(min, coords[i]);
        max = Max(max, coords[i]);
    }
}

//glm::quat QuatFromVectors(NVec3f u, NVec3f v);
NVec4f RectClip(const NVec4f& rect, const NVec4f& clip);
float Luminance(const NVec4f& color);
float Luminance(const NVec3f& color);
float LuminanceABGR(const uint32_t& color);
float LuminanceARGB(const uint32_t& color);
NVec4f Saturate(const NVec4f& col);
NVec4f Desaturate(const NVec4f& col);

template <typename T>
T Clamp(const T& val, const T& min, const T& max)
{
    return std::max(min, std::min(max, val));
}

template <typename T>
inline T AlignUpWithMask(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T>
inline T AlignDownWithMask(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

template <typename T>
inline T AlignUp(T value, size_t alignment)
{
    return AlignUpWithMask(value, alignment - 1);
}

template <typename T>
inline T AlignDown(T value, size_t alignment)
{
    return AlignDownWithMask(value, alignment - 1);
}

template <typename T>
inline bool IsAligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T>
inline T DivideByMultiple(T value, size_t alignment)
{
    return (T)((value + alignment - 1) / alignment);
}

template <typename T>
inline bool IsPowerOfTwo(T value)
{
    return 0 == (value & (value - 1));
}

template <typename T>
inline bool IsDivisible(T value, T divisor)
{
    return (value / divisor) * divisor == value;
}

template <typename T>
inline T Cube(T f)
{
    return f * f * f;
}

template <typename T>
inline T Square(T f)
{
    return f * f;
}

// Transcribed from here: explicit form and derivative
// https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B%C3%A9zier_curves
template <typename T>
inline T Bezier(float t, T p0, T p1, T p2, T p3)
{
    return Cube(1 - t) * p0 + 3 * Square(1 - t) * t * p1 + 3 * (1 - t) * Square(t) * p2 + Cube(t) * p3;
}

template <typename T>
inline NVec2<T> Bezier(float t, const NVec2<T>& p0, const NVec2<T>& p1, const NVec2<T>& p2, const NVec2<T>& p3)
{
    return NVec2<T>(Bezier(t, p0.x, p1.x, p2.x, p3.x), Bezier(t, p0.y, p1.y, p2.y, p3.y));
}

template <typename T>
inline T BezierDerivative(float t, T p0, T p1, T p2, T p3)
{
    return 3 * Square(1 - t) * (p1 - p0) + 6 * (1 - t) * t * (p2 - p1) + 3 * Square(t) * (p3 - p2);
}

// Tangent
template <typename T>
inline NVec2<T> BezierDerivative(float t, const NVec2<T>& p0, const NVec2<T>& p1, const NVec2<T>& p2, const NVec2<T>& p3)
{
    return NVec2<T>(BezierDerivative(t, p0.x, p1.x, p2.x, p3.x), BezierDerivative(t, p0.y, p1.y, p2.y, p3.y));
}

// Normal
template <typename T>
inline NVec2<T> BezierNormal(float t, const NVec2<T>& p0, const NVec2<T>& p1, const NVec2<T>& p2, const NVec2<T>& p3)
{
    auto deriv = BezierDerivative(t, p0, p1, p2, p3);
    return Normalized(NVec2<T>(-deriv.y, deriv.x));
}
/*
inline uint8_t Log2(uint64_t value)
{
    unsigned long mssb; // most significant set bit
    unsigned long lssb; // least significant set bit

    // If perfect power of two (only one set bit), return index of bit.  Otherwise round up
    // fractional log by adding 1 to most signicant set bit's index.
    if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
        return uint8_t(mssb + (mssb == lssb ? 0 : 1));
    else
        return 0;
}

template <typename T> inline T AlignPowerOfTwo(T value)
{
    return value == 0 ? 0 : 1 << Log2(value);
}
*/

} // MUtils

