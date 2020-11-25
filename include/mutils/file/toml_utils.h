#pragma once

#include <map> 
#include <toml.hpp>

#include <mutils/file/file.h>
#include <mutils/math/math.h>

namespace MUtils
{

std::map<std::string, const toml::table&> toml_get_subtables(const toml::value& loaded, std::string parentName, std::function<void(const std::string&, toml::table&)> cb);

std::vector<float> toml_floats(const toml::value& val);
std::vector<int> toml_ints(const toml::value& val);
NVec3f toml_get_vec3(const toml::table& table, const char* pszName, const NVec3f& def = NVec3f(0.0f));
NVec4f toml_get_vec4(const toml::table& table, const char* pszName, const NVec4f& def = NVec4f(0.0f));
NVec4i toml_get_vec4i(const toml::table& table, const char* pszName, const NVec4i& def = NVec4i(0));

const toml::table toml_get_table(const toml::table& table, const char* child);
int toml_line(const toml::table& table, const char* pszName);

toml::value toml_read(const fs::path& path);
void toml_sanitize(std::string& text);
int toml_extract_error_line(std::string err);

template <typename T>
void toml_set(toml::table& table, const char* pszName, const T& val = T{})
{
    auto itr = table.find(pszName);
    if (itr != table.end())
    {
        //return toml::set<T>(itr->second, val);
        assert(!"Implement me!");
    }
    table[pszName] = val;
}

template <typename T>
T toml_get(const toml::table& table, const char* pszName, const T& val = T{})
{
    auto itr = table.find(pszName);
    if (itr != table.end())
    {
        return toml::get<T>(itr->second);
    }
    return val;
}

} // namespace MUtils
