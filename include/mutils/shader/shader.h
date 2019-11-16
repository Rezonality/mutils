#pragma once

#include "mutils/compile/compile_messages.h"
#include "mutils/compile/meta_tags.h"

namespace MUtils
{
enum class ShaderType
{
    Unknown,
    Pixel,
    Vertex,
    Geometry,
    Compute,
};

struct ShaderFragment
{
    std::string name;
    std::string source;
};

struct ShaderPackage
{
    ShaderPackage(const fs::path& p)
        : path(p)
    {}

    std::vector<ShaderFragment> fragments;
    uint32_t mainFragment = 0;
    fs::path path;
    MUtils::MetaTags_t tags;
};

} // namespace MUtils
