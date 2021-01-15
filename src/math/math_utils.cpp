#include <cmath>

#include <mutils/math/math.h>
#include <mutils/math/math_utils.h>
#include <mutils/math/random_utils.h>

static std::random_device rd;
static std::mt19937 mt(rd());

namespace MUtils
{
double RandRange(double min, double max)
{
    std::uniform_real_distribution<double> dist(min, std::nextafter(max, std::numeric_limits<double>::max()));
    return dist(mt);
}

float RandRange(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, std::nextafter(max, std::numeric_limits<float>::max()));
    return dist(mt);
}

uint32_t RandRange(uint32_t min, uint32_t max)
{
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(mt);
}

int32_t RandRange(int32_t min, int32_t max)
{
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(mt);
}

/*
// Build a unit quaternion representing the rotation
// from u to v. The input vectors need not be normalised.
glm::quat QuatFromVectors(NVec3f u, NVec3f v)
{
    float norm_u_norm_v = sqrt(dot(u, u) * dot(v, v));
    float real_part = norm_u_norm_v + dot(u, v);
    NVec3f w;

    if (real_part < 1.e-6f * norm_u_norm_v)
    {
        // If u and v are exactly opposite, rotate 180 degrees
        // around an arbitrary orthogonal axis. Axis normalisation
        // can happen later, when we normalise the quaternion.
        real_part = 0.0f;
        w = std::abs(u.x) > std::abs(u.z) ? NVec3f(-u.y, u.x, 0.f)
            : NVec3f(0.f, -u.z, u.y);
    }
    else
    {
        // Otherwise, build quaternion the standard way. 
        w = cross(u, v);
    }

    return glm::normalize(glm::quat(real_part, w.x, w.y, w.z));
}
*/

float SmoothStep(float val)
{
    return val * val * (3.0f - 2.0f * val);
}

NVec4f RectClip(const NVec4f& rect, const NVec4f& clip)
{
    NVec4f ret = rect;
    if (ret.x < clip.x)
    {
        ret.x = clip.x;
    }
    if (ret.y < clip.y)
    {
        ret.y = clip.y;
    }
    if ((ret.x + ret.z) > (clip.x + clip.z))
    {
        ret.z = clip.z + clip.x - ret.x;
    }
    if ((ret.y + ret.w) > (clip.y + clip.w))
    {
        ret.w = clip.w + clip.y - ret.y;
    }

    return ret;
}

float LuminanceABGR(const uint32_t& color)
{
    float red = (color & 0xFF) * .299f;
    float green = ((color & 0xFF00) >> 8) * .587f;
    float blue = ((color & 0xFF0000) >> 16) * .114f;
    return (red + green + blue) / float(255);
}

float LuminanceARGB(const uint32_t& color)
{
    float blue = (color & 0xFF) * .299f;
    float green = ((color & 0xFF00) >> 8) * .587f;
    float red = ((color & 0xFF0000) >> 16) * .114f;
    return (red + green + blue) / float(255);
}

float Luminance(const NVec4f& color)
{
    return Luminance(NVec3f(color.x, color.y, color.z) * color.w);
}

float Luminance(const NVec3f& color)
{
    // Perceived.
    return (0.299f * color.x + 0.587f * color.y + 0.114f * color.z);
}

NVec4f Desaturate(const NVec4f& col)
{
    float r = col.x;
    float g = col.y;
    float b = col.z;

    r += .5f;
    g += .5f;
    b += .5f;

    auto sq = sqrtf((r * r) + (g * g) + (b * b));
    if (sq != 0.0f)
        sq = 1.0f / sq;

    r = r * sq;
    g = g * sq;
    b = b * sq;

    return NVec4f(r, g, b, col.w);
}

NVec4f Saturate(const NVec4f& col)
{
    float r = col.x;
    float g = col.y;
    float b = col.z;

    r -= .25f;
    g -= .25f;
    b -= .25f;

    if (r < 0.0f)
        r = 0.0f;
    if (g < 0.0f)
        g = 0.0f;
    if (b < 0.0f)
        b = 0.0f;
    return NVec4f(r, g, b, col.w);
}

} // namespace MUtils
