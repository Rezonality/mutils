#pragma once

#include <glm/glm.hpp>
#include <algorithm>

float SmoothStep(float val);

template<class T>
bool IsRectEmpty(const T& rect)
{
    return (rect.z == 0 || rect.w == 0);
}

template<typename T, typename P> 
bool RectContains(const T& rect, const P& point)
{
    return ((rect.x <= point.x && (rect.x + rect.z) >= point.x) &&
        (rect.y <= point.y && (rect.y + rect.w) >= point.y));
}

double RandRange(double begin, double end);
float RandRange(float begin, float end);
uint32_t RandRange(uint32_t min, uint32_t max);
int32_t RandRange(int32_t begin, int32_t end);

void GetBounds(const glm::vec3* coords, uint32_t count, glm::vec3& min, glm::vec3& max);
glm::quat QuatFromVectors(glm::vec3 u, glm::vec3 v);
glm::vec4 RectClip(const glm::vec4& rect, const glm::vec4& clip);
float Luminance(const glm::vec4& color);
float Luminance(const glm::vec3& color);
float LuminanceABGR(const uint32_t& color);
float LuminanceARGB(const uint32_t& color);
glm::vec4 Saturate(const glm::vec4& col);
glm::vec4 Desaturate(const glm::vec4& col);

template<typename T>
T Clamp(const T &val, const T &min, const T &max)
{
    return std::max(min, std::min(max, val));
}

template <typename T> inline T AlignUpWithMask( T value, size_t mask )
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T> inline T AlignDownWithMask( T value, size_t mask )
{
    return (T)((size_t)value & ~mask);
}

template <typename T> inline T AlignUp( T value, size_t alignment )
{
    return AlignUpWithMask(value, alignment - 1);
}

template <typename T> inline T AlignDown( T value, size_t alignment )
{
    return AlignDownWithMask(value, alignment - 1);
}

template <typename T> inline bool IsAligned( T value, size_t alignment )
{
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T> inline T DivideByMultiple( T value, size_t alignment )
{
    return (T)((value + alignment - 1) / alignment);
}

template <typename T> inline bool IsPowerOfTwo(T value)
{
    return 0 == (value & (value - 1));
}

template <typename T> inline bool IsDivisible(T value, T divisor)
{
    return (value / divisor) * divisor == value;
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

