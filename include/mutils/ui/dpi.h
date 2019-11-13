#pragma once

namespace MUtils
{

struct Dpi
{
    float scaleFactor = 1.0;
};
extern Dpi dpi;

void check_dpi();

} // MUtils
