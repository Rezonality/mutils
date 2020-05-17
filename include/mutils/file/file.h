#pragma once

#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

namespace MUtils
{

std::string file_read(const fs::path& fileName);
bool file_write(const fs::path& fileName, const void* pData, size_t size);
bool file_write(const fs::path& fileName, const std::string& str);
fs::path file_get_relative_path(fs::path from, fs::path to);
std::vector<fs::path> file_gather_files(const fs::path& root);

fs::path file_exe_path();
fs::path file_documents_path();
fs::path file_roaming_path();
fs::path file_appdata_path();

} // namespace MUtils
