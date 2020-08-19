#pragma once

#include <cstdint>
#include <mutils/math/math.h>

namespace MUtils
{

struct Fbo
{
    uint32_t fbo = 0;
    uint32_t texture = 0;
    uint32_t depth = 0;
    NVec2i size;
    NVec2i displaySize;
};

void fbo_bind(const Fbo& fbo);
void fbo_unbind(const Fbo& fbo, const NVec2i& displaySize);
Fbo fbo_create();
void fbo_resize(Fbo& fbo, const NVec2i& size);
void fbo_clear(const NVec4f& color);
void fbo_destroy(const Fbo& fbo);

} // namespace MUtils
