#pragma once

#include <mutils/ui/dpi.h>
#include <mutils/math/math.h>

namespace MUtils
{

inline void sdl_update_dpi()
{
    float ddpi = 0.0f;
    float hdpi = 0.0f;
    float vdpi = 0.0f;

    auto win = SDL_GL_GetCurrentWindow();
    auto index = win ? SDL_GetWindowDisplayIndex(win) : 0;
    auto res = SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);

    NVec2f dpiScale;
    if (res == 0 && hdpi != 0)
    {
        dpiScale = NVec2f(hdpi, vdpi) / 96.0f;
    }
    else
    {
        dpiScale = NVec2f(1.0f);
    }

    set_dpi(dpiScale);
}


} // namespace MUtils
