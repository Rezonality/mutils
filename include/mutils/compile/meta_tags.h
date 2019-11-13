#pragma once

#include <string>
#include <memory>
#include <map>
#include <cstdint>

namespace MUtils
{

struct TagValue
{
    std::string value;
    int32_t line = -1;
};

using MetaTags_t = std::map<std::string, TagValue>;
MetaTags_t parse_meta_tags(const std::string& text);

};
