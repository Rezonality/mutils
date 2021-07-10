#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <ostream>

// This just saves using a library like glm (my personal preference)
namespace MUtils
{

const inline float ZPI = 3.14159862f;
template <class T>
struct NVec2
{
    NVec2(T xVal, T yVal)
        : x(xVal)
        , y(yVal)
    {
    }

    NVec2(T v)
        : x(v)
        , y(v)
    {
    }

    template <class Y>
    NVec2(NVec2<Y> val)
        : x((T)val.x)
        , y((T)val.y)
    {
    }

    NVec2()
        : x(0)
        , y(0)
    {
    }

    T x;
    T y;

    bool operator==(const NVec2<T>& rhs) const
    {
        if (x == rhs.x && y == rhs.y)
            return true;
        return false;
    }

    bool operator!=(const NVec2<T>& rhs) const
    {
        return !(*this == rhs);
    }
};

template <class T>
inline NVec2<T> Abs(const NVec2<T>& lhs)
{
    return NVec2<T>(std::abs(lhs.x), std::abs(lhs.y));
}
template <class T>
inline NVec2<T> Min(const NVec2<T>& lhs, const NVec2<T>& rhs)
{
    return NVec2<T>(std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y));
}
template <class T>
inline NVec2<T> Max(const NVec2<T>& lhs, const NVec2<T>& rhs)
{
    return NVec2<T>(std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y));
}

template <class T>
inline NVec2<T> operator+(const NVec2<T>& lhs, const NVec2<T>& rhs)
{
    return NVec2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}
template <class T>
inline NVec2<T> operator-(const NVec2<T>& lhs, const NVec2<T>& rhs)
{
    return NVec2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}
template <class T>
inline NVec2<T> operator/(const NVec2<T>& lhs, const NVec2<T>& rhs)
{
    return NVec2<T>(lhs.x / rhs.x, lhs.y / rhs.y);
}
template <class T>
inline NVec2<T>& operator+=(NVec2<T>& lhs, const NVec2<T>& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}
template <class T>
inline NVec2<T>& operator-=(NVec2<T>& lhs, const NVec2<T>& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}
template <class T>
inline NVec2<T> operator*(const NVec2<T>& lhs, float val)
{
    return NVec2<T>(lhs.x * val, lhs.y * val);
}
template <class T>
inline NVec2<T> operator*(const NVec2<T>& lhs, const NVec2<T>& rhs)
{
    return NVec2<T>(lhs.x * rhs.x, lhs.y * rhs.y);
}
template <class T>
inline NVec2<T> operator/(const NVec2<T>& lhs, float val)
{
    return NVec2<T>(lhs.x / val, lhs.y / val);
}
template <class T>
inline NVec2<T>& operator*=(NVec2<T>& lhs, float val)
{
    lhs.x *= val;
    lhs.y *= val;
    return lhs;
}
template <class T>
inline bool operator<(const NVec2<T>& lhs, const NVec2<T>& rhs)
{
    if (lhs.y != rhs.y)
    {
        return lhs.y < rhs.y;
    }
    return lhs.x < rhs.x;
}
template <class T>
inline NVec2<T> Clamp(const NVec2<T>& val, const NVec2<T>& min, const NVec2<T>& max)
{
    return NVec2<T>(std::min(max.x, std::max(min.x, val.x)), std::min(max.y, std::max(min.y, val.y)));
}
template <class T>
inline T ManhattanDistance(const NVec2<T>& l, const NVec2<T>& r)
{
    return std::abs(l.x - r.x) + std::abs(r.y - l.y);
}

template <class T>
inline T LengthSquared(const NVec2<T>& v)
{
    return (v.x * v.x) + (v.y * v.y);
}

template <class T>
inline T Length(const NVec2<T>& v)
{
    return sqrt(LengthSquared(v));
}

template <class T>
inline NVec2<T> Normalized(const NVec2<T>& val)
{
    return val / Length(val);
}

template <class T>
    std::ostream& operator<<(std::ostream& str, const NVec2<T>& v)
{
    str << "(" << v.x << ", " << v.y << ")";
    return str;
}

using NVec2f = NVec2<float>;
using NVec2d = NVec2<double>;
using NVec2i = NVec2<int>;
using NVec2l = NVec2<long>;
using NVec2ll = NVec2<int64_t>;
using NVec2ull = NVec2<uint64_t>;

template <class T>
struct NVec3
{
    NVec3(T xVal, T yVal, T zVal)
        : x(xVal)
        , y(yVal)
        , z(zVal)
    {
    }

    explicit NVec3(T val)
        : NVec3(val, val, val)
    {
    }

    template <class Y>
    explicit NVec3(NVec3<Y> val)
        : x((T)val.x)
        , y((T)val.y)
        , z((T)val.z)
    {
    }

    NVec3()
        : x(0)
        , y(0)
        , z(0)
    {
    }

    T x;
    T y;
    T z;

    bool operator==(const NVec3<T>& rhs) const
    {
        if (x == rhs.x && y == rhs.y && z == rhs.z)
            return true;
        return false;
    }

    bool operator!=(const NVec3<T>& rhs) const
    {
        return !(*this == rhs);
    }

    T& operator[](size_t index)
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            assert(!"Invalid index");
            return x;
        }
    }

    const T& operator[](size_t index) const
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            assert(!"Invalid index");
            return x;
        }
    }
};
template <class T>
inline NVec3<T> Abs(const NVec3<T>& lhs)
{
    return NVec3<T>(std::abs(lhs.x), std::abs(lhs.y), std::abs(lhs.z));
}
template <class T>
inline NVec3<T> Min(const NVec3<T>& lhs, const NVec3<T>& rhs)
{
    return NVec3<T>(std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z));
}
template <class T>
inline NVec3<T> Max(const NVec3<T>& lhs, const NVec3<T>& rhs)
{
    return NVec3<T>(std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z));
}
template <class T>
inline NVec3<T> operator+(const NVec3<T>& lhs, const NVec3<T>& rhs)
{
    return NVec3<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}
template <class T>
inline NVec3<T> operator-(const NVec3<T>& lhs, const NVec3<T>& rhs)
{
    return NVec3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}
template <class T>
inline NVec3<T>& operator+=(NVec3<T>& lhs, const NVec3<T>& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}
template <class T>
inline NVec3<T>& operator-=(NVec3<T>& lhs, const NVec3<T>& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;
}
template <class T>
inline NVec3<T> operator*(const NVec3<T>& lhs, T val)
{
    return NVec3<T>(lhs.x * val, lhs.y * val, lhs.z * val);
}

template <class T>
inline NVec3<T> operator*(const NVec3<T>& lhs, const NVec3<T>& rhs)
{
    return NVec3<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

template <class T>
inline NVec3<T> operator/(const NVec3<T>& lhs, T val)
{
    return NVec3<T>(lhs.x / val, lhs.y / val, lhs.z / val);
}

template <class T>
inline NVec3<T> operator/(const NVec3<T>& lhs, const NVec3<T>& rhs)
{
    return NVec3<T>(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
}

template <class T>
inline NVec3<T>& operator*=(NVec3<T>& lhs, float val)
{
    lhs.x *= val;
    lhs.y *= val;
    lhs.z *= val;
    return lhs;
}

template <class T>
inline NVec3<T>& operator*=(NVec3<T>& lhs, const NVec3<T>& rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    return lhs;
}

template <class T>
inline NVec3<T>& operator/=(NVec3<T>& lhs, T val)
{
    lhs.x /= val;
    lhs.y /= val;
    lhs.z /= val;
    return lhs;
}

template <class T>
inline NVec3<T>& operator/=(NVec3<T>& lhs, const NVec3<T>& rhs)
{
    lhs.x /= rhs.x;
    lhs.y /= rhs.y;
    lhs.z /= rhs.z;
    return lhs;
}
template <class T>
inline NVec3<T> Clamp(const NVec3<T>& val, const NVec3<T>& min, const NVec3<T>& max)
{
    return NVec3<T>(std::min(max.x, std::max(min.x, val.x)),
        std::min(max.y, std::max(min.y, val.y)),
        std::min(max.z, std::max(min.z, val.z)));
}

template <class T>
inline T LengthSquared(const NVec3<T>& v)
{
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

template <class T>
inline T Length(const NVec3<T>& v)
{
    return sqrt(LengthSquared(v));
}

template <class T>
inline NVec3<T> Normalized(const NVec3<T>& val)
{
    return val / Length(val);
}
inline uint32_t ToPacked(const NVec3<float>& val)
{
    uint32_t col = 0;
    col |= uint32_t(val.x * 255.0f) << 24;
    col |= uint32_t(val.y * 255.0f) << 16;
    col |= uint32_t(val.z * 255.0f) << 8;
    col |= 0xFF;
    return col;
}

inline uint32_t ToPackedARGB(const NVec3<float>& val)
{
    uint32_t col = 0;
    col |= 0xFF000000;
    col |= uint32_t(val.x * 255.0f) << 16;
    col |= uint32_t(val.y * 255.0f) << 8;
    col |= uint32_t(val.z * 255.0f);
    return col;
}

inline uint32_t ToPackedABGR(const NVec3<float>& val)
{
    uint32_t col = 0;
    col |= 0xFF000000;
    col |= uint32_t(val.x * 255.0f);
    col |= uint32_t(val.y * 255.0f) << 8;
    col |= uint32_t(val.z * 255.0f) << 16;
    return col;
}

inline uint32_t ToPackedBGRA(const NVec3<float>& val)
{
    uint32_t col = 0;
    col |= uint32_t(0xFF);
    col |= uint32_t(val.x * 255.0f) << 16;
    col |= uint32_t(val.y * 255.0f) << 24;
    col |= uint32_t(val.z * 255.0f) << 8;
    return col;
}

inline float Luminosity(const NVec3<float>& intensity)
{
    return (0.2126f * intensity.x + 0.7152f * intensity.y + 0.0722f * intensity.z);
}

inline NVec3<float> Mix(const NVec3<float>& c1, const NVec3<float>& c2, float factor)
{
    NVec3<float> ret = c1 * (1.0f - factor);
    ret = ret + (c2 * factor);
    return ret;
}

template <class T>
inline std::ostream& operator<<(std::ostream& str, const NVec3<T>& region)
{
    str << "(" << region.x << ", " << region.y << ", " << region.z << ")";
    return str;
}

using NVec3f = NVec3<float>;
using NVec3d = NVec3<double>;
using NVec3i = NVec3<int>;
using NVec3l = NVec3<long>;
using NVec3ll = NVec3<int64_t>;
using NVec3ull = NVec3<uint64_t>;

template <class T>
struct NVec4
{
    NVec4(T xVal, T yVal, T zVal, T wVal)
        : x(xVal)
        , y(yVal)
        , z(zVal)
        , w(wVal)
    {
    }

    explicit NVec4(T val)
        : NVec4(val, val, val, val)
    {
    }

    template <class Y>
    explicit NVec4(NVec4<Y> val)
        : x((T)val.x)
        , y((T)val.y)
        , z((T)val.z)
        , w((T)val.w)
    {
    }

    NVec4()
        : x(0)
        , y(0)
        , z(0)
        , w(1)
    {
    }

    T x;
    T y;
    T z;
    T w;

    bool operator==(const NVec4<T>& rhs) const
    {
        if (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w)
            return true;
        return false;
    }

    bool operator!=(const NVec4<T>& rhs) const
    {
        return !(*this == rhs);
    }

    T& operator[](size_t index)
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            assert(!"Invalid index");
            return x;
        }
    }

    const T& operator[](size_t index) const
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            assert(!"Invalid index");
            return x;
        }
    }
};
template <class T>
inline NVec4<T> Abs(const NVec4<T>& lhs)
{
    return NVec4<T>(std::abs(lhs.x), std::abs(lhs.y), std::abs(lhs.z), std::abs(lhs.w));
}
template <class T>
inline NVec4<T> Min(const NVec4<T>& lhs, const NVec4<T>& rhs)
{
    return NVec4<T>(std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z), std::min(lhs.w, rhs.w));
}
template <class T>
inline NVec4<T> Max(const NVec4<T>& lhs, const NVec4<T>& rhs)
{
    return NVec4<T>(std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z), std::max(lhs.w, rhs.w));
}
template <class T>
inline NVec4<T> operator+(const NVec4<T>& lhs, const NVec4<T>& rhs)
{
    return NVec4<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}
template <class T>
inline NVec4<T> operator-(const NVec4<T>& lhs, const NVec4<T>& rhs)
{
    return NVec4<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}
template <class T>
inline NVec4<T> operator-(const NVec4<T>& lhs)
{
    return NVec4<T>(-lhs.x, -lhs.y, -lhs.z, -lhs.w);
}
template <class T>
inline NVec4<T>& operator+=(NVec4<T>& lhs, const NVec4<T>& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    lhs.w += rhs.w;
    return lhs;
}
template <class T>
inline NVec4<T>& operator-=(NVec4<T>& lhs, const NVec4<T>& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    lhs.w -= rhs.w;
    return lhs;
}
template <class T>
inline NVec4<T> operator*(const NVec4<T>& lhs, T val)
{
    return NVec4<T>(lhs.x * val, lhs.y * val, lhs.z * val, lhs.w * val);
}

template <class T>
inline NVec4<T> operator*(const NVec4<T>& lhs, const NVec4<T>& rhs)
{
    return NVec4<T>(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}

template <class T>
inline NVec4<T> operator/(const NVec4<T>& lhs, T val)
{
    return NVec4<T>(lhs.x / val, lhs.y / val, lhs.z / val, lhs.w / val);
}

template <class T>
inline NVec4<T> operator/(const NVec4<T>& lhs, const NVec4<T>& rhs)
{
    return NVec4<T>(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
}

template <class T>
inline NVec4<T>& operator*=(NVec4<T>& lhs, float val)
{
    lhs.x *= val;
    lhs.y *= val;
    lhs.z *= val;
    lhs.w *= val;
    return lhs;
}

template <class T>
inline NVec4<T>& operator*=(NVec4<T>& lhs, const NVec4<T>& rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    lhs.w *= rhs.w;
    return lhs;
}

template <class T>
inline NVec4<T>& operator/=(NVec4<T>& lhs, T val)
{
    lhs.x /= val;
    lhs.y /= val;
    lhs.z /= val;
    lhs.w /= val;
    return lhs;
}

template <class T>
inline NVec4<T>& operator/=(NVec4<T>& lhs, const NVec4<T>& rhs)
{
    lhs.x /= rhs.x;
    lhs.y /= rhs.y;
    lhs.z /= rhs.z;
    lhs.w /= rhs.w;
    return lhs;
}
template <class T>
inline NVec4<T> Clamp(const NVec4<T>& val, const NVec4<T>& min, const NVec4<T>& max)
{
    return NVec4<T>(std::min(max.x, std::max(min.x, val.x)),
        std::min(max.y, std::max(min.y, val.y)),
        std::min(max.z, std::max(min.z, val.z)),
        std::min(max.w, std::max(min.w, val.w)));
}

inline uint32_t ToPacked(const NVec4<float>& val)
{
    uint32_t col = 0;
    col |= uint32_t(val.x * 255.0f) << 24;
    col |= uint32_t(val.y * 255.0f) << 16;
    col |= uint32_t(val.z * 255.0f) << 8;
    col |= uint32_t(val.w * 255.0f);
    return col;
}

inline uint32_t ToPackedARGB(const NVec4<float>& val)
{
    uint32_t col = 0;
    col |= uint32_t(val.w * 255.0f) << 24;
    col |= uint32_t(val.x * 255.0f) << 16;
    col |= uint32_t(val.y * 255.0f) << 8;
    col |= uint32_t(val.z * 255.0f);
    return col;
}

inline uint32_t ToPackedABGR(const NVec4<float>& val)
{
    uint32_t col = 0;
    col |= uint32_t(val.w * 255.0f) << 24;
    col |= uint32_t(val.x * 255.0f);
    col |= uint32_t(val.y * 255.0f) << 8;
    col |= uint32_t(val.z * 255.0f) << 16;
    return col;
}

inline uint32_t ToPackedBGRA(const NVec4<float>& val)
{
    uint32_t col = 0;
    col |= uint32_t(val.w * 255.0f) << 8;
    col |= uint32_t(val.x * 255.0f) << 16;
    col |= uint32_t(val.y * 255.0f) << 24;
    col |= uint32_t(val.z * 255.0f);
    return col;
}

inline float Luminosity(const NVec4<float>& intensity)
{
    return (0.2126f * intensity.x + 0.7152f * intensity.y + 0.0722f * intensity.z);
}

inline NVec4<float> Mix(const NVec4<float>& c1, const NVec4<float>& c2, float factor)
{
    NVec4<float> ret = c1 * (1.0f - factor);
    ret = ret + (c2 * factor);
    return ret;
}

inline NVec4<float> HSVToRGB(float h, float s, float v)
{
    auto r = 0.0f, g = 0.0f, b = 0.0f;

    if (s == 0)
    {
        r = v;
        g = v;
        b = v;
    }
    else
    {
        int i;
        float f, p, q, t;

        if (h == 360)
            h = 0;
        else
            h = h / 60.0f;

        i = (int)trunc(h);
        f = h - i;

        p = v * (1.0f - s);
        q = v * (1.0f - (s * f));
        t = v * (1.0f - (s * (1.0f - f)));

        switch (i)
        {
        case 0:
            r = v;
            g = t;
            b = p;
            break;

        case 1:
            r = q;
            g = v;
            b = p;
            break;

        case 2:
            r = p;
            g = v;
            b = t;
            break;

        case 3:
            r = p;
            g = q;
            b = v;
            break;

        case 4:
            r = t;
            g = p;
            b = v;
            break;

        default:
            r = v;
            g = p;
            b = q;
            break;
        }
    }

    return NVec4<float>(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

template <class T>
inline std::ostream& operator<<(std::ostream& str, const NVec4<T>& region)
{
    str << "(" << region.x << ", " << region.y << ", " << region.z << ", " << region.w << ")";
    return str;
}

using NVec4f = NVec4<float>;
using NVec4d = NVec4<double>;
using NVec4i = NVec4<int>;
using NVec4l = NVec4<long>;

template <class T>
struct NRect
{
    NRect(const NVec2<T>& topLeft, const NVec2<T>& bottomRight)
        : topLeftPx(topLeft)
        , bottomRightPx(bottomRight)
    {
    }

    NRect(T left, T top, T width, T height)
        : topLeftPx(NVec2<T>(left, top))
        , bottomRightPx(NVec2<T>(left, top) + NVec2<T>(width, height))
    {
    }

    NRect()
    {
    }

    NVec2<T> topLeftPx;
    NVec2<T> bottomRightPx;

    bool Contains(const NVec2<T>& pt) const
    {
        return topLeftPx.x <= pt.x && topLeftPx.y <= pt.y && bottomRightPx.x > pt.x && bottomRightPx.y > pt.y;
    }

    NVec2<T> BottomLeft() const
    {
        return NVec2<T>(topLeftPx.x, bottomRightPx.y);
    }
    NVec2<T> TopRight() const
    {
        return NVec2<T>(bottomRightPx.x, topLeftPx.y);
    }

    T Left() const
    {
        return topLeftPx.x;
    }
    T Right() const
    {
        return TopRight().x;
    }
    T Top() const
    {
        return TopRight().y;
    }
    T Bottom() const
    {
        return bottomRightPx.y;
    }
    T Height() const
    {
        return bottomRightPx.y - topLeftPx.y;
    }
    T Width() const
    {
        return bottomRightPx.x - topLeftPx.x;
    }
    NVec2<T> Center() const
    {
        return (bottomRightPx + topLeftPx) * .5f;
    }
    NVec2<T> Size() const
    {
        return bottomRightPx - topLeftPx;
    }
    bool Empty() const
    {
        return (Height() == 0.0f || Width() == 0.0f) ? true : false;
    }

    void Clear()
    {
        topLeftPx = NVec2<T>();
        bottomRightPx = NVec2<T>();
    }

    void Normalize()
    {
        if (topLeftPx.x > bottomRightPx.x)
        {
            std::swap(topLeftPx.x, bottomRightPx.x);
        }
        if (topLeftPx.y > bottomRightPx.y)
        {
            std::swap(topLeftPx.y, bottomRightPx.y);
        }
    }

    void SetSize(const NVec2<T>& size)
    {
        bottomRightPx = topLeftPx + size;
    }

    NRect<T>& Adjust(T x, T y, T z, T w)
    {
        topLeftPx.x += x;
        topLeftPx.y += y;
        bottomRightPx.x += z;
        bottomRightPx.y += w;
        return *this;
    }

    NRect<T>& Adjust(T x, T y)
    {
        topLeftPx.x += x;
        topLeftPx.y += y;
        bottomRightPx.x += x;
        bottomRightPx.y += y;
        return *this;
    }

    NRect<T>& Adjust(const NVec2<T>& v)
    {
        return Adjust(v.x, v.y);
    }

    NRect<T>& Adjust(const NVec4<T>& v)
    {
        return Adjust(v.x, v.y, v.z, v.w);
    }

    NRect<T> Adjusted(const NVec4<T>& v) const
    {
        NRect<T> adj(topLeftPx, bottomRightPx);
        adj.Adjust(v);
        return adj;
    }
    
    NRect<T> Adjusted(const NVec2<T>& v) const
    {
        NRect<T> adj(topLeftPx, bottomRightPx);
        adj.Adjust(v);
        return adj;
    }

    NRect<T>& Move(T x, T y)
    {
        auto width = Width();
        auto height = Height();
        topLeftPx.x = x;
        topLeftPx.y = y;
        bottomRightPx.x = x + width;
        bottomRightPx.y = y + height;
        return *this;
    }

    NRect<T>& Move(const NVec2<T>& v)
    {
        return Move(v.x, v.y);
    }

    NRect<T> Moved(const NVec2<T>& v) const
    {
        NRect<T> adj(topLeftPx, bottomRightPx);
        adj.Move(v);
        return adj;
    }

    NRect<T>& Expand(T x, T y, T z, T w)
    {
        topLeftPx.x -= x;
        topLeftPx.y -= y;
        bottomRightPx.x += z;
        bottomRightPx.y += w;
        return *this;
    }

    NRect<T>& Expand(const NVec4<T> v)
    {
        return Expand(v.x, v.y, v.z, v.w);
    }

    NRect<T> Expanded(const NVec4<T> v) const
    {
        NRect<T> adj(topLeftPx, bottomRightPx);
        adj.Expand(v);
        return adj;
    }

    NRect<T>& Clamp(const NRect<T>& r)
    {
        T x, y, z, w;
        x = topLeftPx.x;
        y = topLeftPx.y;
        z = Width();
        w = Height();
        if (x > r.Right())
        {
            x = r.Right();
            z = 0.0;
        }
        else if (x < r.Left())
        {
            z = (x + z) - r.Left();
            x = r.Left();
            z = std::max(0.0f, z);
        }

        if (y > r.Bottom())
        {
            y = r.Bottom();
            w = 0.0;
        }
        else if (y < r.Top())
        {
            w = (y + w) - r.Top();
            y = r.Top();
            w = std::max(0.0f, w);
        }

        if ((x + z) >= r.Right())
        {
            z = r.Right() - x;
            z = std::max(0.0f, z);
        }

        if ((y + w) >= r.Bottom())
        {
            w = r.Bottom() - y;
            w = std::max(0.0f, w);
        }
        topLeftPx = NVec2<T>(x, y);
        bottomRightPx = NVec2<T>(x + z, y + w);
        return *this;
    }

    bool operator==(const NRect<T>& region) const
    {
        return (topLeftPx == region.topLeftPx) && (bottomRightPx == region.bottomRightPx);
    }
    bool operator!=(const NRect<T>& region) const
    {
        return !(*this == region);
    }
};

template <class T>
inline NRect<T> operator*(const NRect<T>& lhs, float val)
{
    return NRect<T>(lhs.topLeftPx * val, lhs.bottomRightPx * val);
}
template <class T>
inline NRect<T> operator*(const NRect<T>& lhs, const NVec2f& val)
{
    return NRect<T>(lhs.topLeftPx * val.x, lhs.bottomRightPx * val.y);
}
template <class T>
inline NRect<T> operator-(const NRect<T>& lhs, const NRect<T>& rhs)
{
    return NRect<T>(lhs.topLeftPx.x - rhs.topLeftPx.x, lhs.bottomRightPx.y - rhs.topLeftPx.y);
}
template <class T>
inline NRect<T> operator-(const NRect<T>& lhs, const NVec2<T>& pos)
{
    return lhs.Adjusted(-pos);
}
template <class T>
inline NRect<T> operator+(const NRect<T>& lhs, const NVec2<T>& pos)
{
    return lhs.Adjusted(pos);
}
template <class T>
inline std::ostream& operator<<(std::ostream& str, const NRect<T>& region)
{
    str << region.topLeftPx << ", " << region.bottomRightPx << ", size: " << region.Width() << ", " << region.Height();
    return str;
}

enum FitCriteria
{
    X,
    Y
};

template <class T>
inline bool NRectFits(const NRect<T>& area, const NRect<T>& rect, FitCriteria criteria)
{
    if (criteria == FitCriteria::X)
    {
        auto xDiff = rect.bottomRightPx.x - area.bottomRightPx.x;
        if (xDiff > 0)
        {
            return false;
        }
        xDiff = rect.topLeftPx.x - area.topLeftPx.x;
        if (xDiff < 0)
        {
            return false;
        }
    }
    else
    {
        auto yDiff = rect.bottomRightPx.y - area.bottomRightPx.y;
        if (yDiff > 0)
        {
            return false;
        }
        yDiff = rect.topLeftPx.y - area.topLeftPx.y;
        if (yDiff < 0)
        {
            return false;
        }
    }
    return true;
}

using NRectf = NRect<float>;
using NRecti = NRect<long>;

inline float ZClamp(float x, float lowerlimit, float upperlimit)
{
    x = std::max(lowerlimit, x);
    x = std::min(upperlimit, x);
    return x;
}

inline float ZSmoothStep(float edge0, float edge1, float x)
{
    // Scale, bias and saturate x to 0..1 range
    x = ZClamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

inline void hash_combine(size_t& seed, size_t hash)
{
    hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash;
}

inline float degToRad(float deg)
{
    return deg / 180.0f * 3.1415926f;
}

inline float radToDeg(float rad)
{
    return rad / 3.1415926f * 180.0f;
}

} // namespace MUtils

namespace std
{

template <class T>
struct hash<MUtils::NVec2<T>>
{
    inline size_t operator()(const MUtils::NVec2<T>& v) const
    {
        size_t seed = 0;
        hash<T> hasher;
        MUtils::hash_combine(seed, hasher(v.x));
        MUtils::hash_combine(seed, hasher(v.y));
        return seed;
    };
};

template <class T>
struct hash<MUtils::NVec4<T>>
{
    inline size_t operator()(const MUtils::NVec4<T>& v) const
    {
        size_t seed = 0;
        hash<T> hasher;
        MUtils::hash_combine(seed, hasher(v.x));
        MUtils::hash_combine(seed, hasher(v.y));
        MUtils::hash_combine(seed, hasher(v.z));
        MUtils::hash_combine(seed, hasher(v.w));
        return seed;
    };
};

template <class T>
struct hash<MUtils::NRect<T>>
{
    inline size_t operator()(const MUtils::NRect<T>& v) const
    {
        size_t seed = 0;
        hash<T> hasher;
        MUtils::hash_combine(seed, hasher(v.topLeftPx));
        MUtils::hash_combine(seed, hasher(v.bottomRightPx));
        return seed;
    };
};

}; // namespace std
