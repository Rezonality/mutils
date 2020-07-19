#pragma once

#include <mutils/math/math.h>

namespace MUtils
{

struct Dpi
{
    float scaleFactor = 1.0;
    NVec2f scaleFactorXY = NVec2f(1.0f);
};
extern Dpi dpi;

void check_dpi();
void set_dpi(const NVec2f& val);
float dpi_pixel_height_from_point_size(float pointSize, float pixelScaleY);

} // MUtils
