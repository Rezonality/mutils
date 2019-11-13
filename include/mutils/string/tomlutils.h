#pragma once

#include <string>

namespace MUtils
{
void sanitize_for_toml(std::string& text);
int extract_toml_error_line(std::string err);
}
