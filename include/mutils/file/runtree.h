#pragma once

#include "mutils/file/file.h"

namespace MUtils
{

void runtree_init(const fs::path& appRoot);
void runtree_destroy();
fs::path runtree_find_asset(const fs::path& p);
std::string runtree_load_asset(const fs::path& p);
fs::path runtree_path();

}
