#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <functional>

#include <toml11/toml.hpp>

#include <mutils/math/math.h>

namespace MUtils
{

inline std::map<std::string, const toml::table&> toml_get_subtables(const toml::value& loaded, std::string parentName, std::function<void(const std::string&, toml::table&)> cb)
{
    std::map<std::string, const toml::table&> ret;

    auto parents = toml::expect<toml::table>(loaded, parentName);
    if (parents.is_ok())
    {
        for (auto& tablePair : parents.unwrap())
        {
            auto tableType = toml::expect<toml::table>(tablePair.second);
            if (tableType.is_err())
            {
                continue;
            }

            cb(tablePair.first, tableType.unwrap());
        }
    }
    return ret;
}

inline std::vector<float> toml_floats(const toml::value& val)
{
    try
    {
        return toml::get<std::vector<float>>(val);
    }
    catch (...)
    {
        auto ret = toml::get<std::vector<int>>(val);
        std::vector<float> vals;
        for (auto& val : ret)
        {
            vals.push_back((float)val);
        }
        return vals;
    }

    return std::vector<float>{};
}

inline std::vector<int> toml_ints(const toml::value& val)
{
    try
    {
        return toml::get<std::vector<int>>(val);
    }
    catch (...)
    {
        auto ret = toml::get<std::vector<int>>(val);
        std::vector<int> vals;
        for (auto& val : ret)
        {
            vals.push_back((int)val);
        }
        return vals;
    }

    return std::vector<int>{};
}


inline NVec3f toml_get_vec3(const toml::table& table, const char* pszName, const NVec3f& def = NVec3f(0.0f))
{
    NVec3f ret = def;
    if (table.count(pszName) != 0)
    {
        auto itrFloats = table.find(pszName);
        if (itrFloats != table.end())
        {
            auto v = toml_floats(itrFloats->second);
            if (v.size() == 3)
            {
                ret.x = v[0];
                ret.y = v[1];
                ret.z = v[2];
            }
            else if (v.size() == 1)
            {
                ret.x = v[0];
                ret.y = v[0];
                ret.z = v[0];
            }
        }
    }
    return ret;
}

inline NVec4f toml_get_vec4(const toml::table& table, const char* pszName, const NVec4f& def = NVec4f(0.0f))
{
    NVec4f ret = def;
    if (table.count(pszName) != 0)
    {
        auto itrFloats = table.find(pszName);
        if (itrFloats != table.end())
        {
            auto v = toml_floats(itrFloats->second);
            if (v.size() == 4)
            {
                ret.x = v[0];
                ret.y = v[1];
                ret.z = v[2];
                ret.w = v[3];
            }
            else if (v.size() == 3)
            {
                ret.x = v[0];
                ret.y = v[1];
                ret.z = v[2];
            }
            else if (v.size() == 2)
            {
                ret.x = v[0];
                ret.y = v[1];
            }
            else if (v.size() == 1)
            {
                ret.x = v[0];
                ret.y = v[0];
                ret.z = v[0];
                ret.w = v[0];
            }
        }
    }
    return ret;
}

inline NVec4i toml_get_vec4i(const toml::table& table, const char* pszName, const NVec4i& def = NVec4i(0))
{
    NVec4i ret = def;
    if (table.count(pszName) != 0)
    {
        auto itrFloats = table.find(pszName);
        if (itrFloats != table.end())
        {
            auto v = toml_ints(itrFloats->second);
            if (v.size() == 4)
            {
                ret.x = v[0];
                ret.y = v[1];
                ret.z = v[2];
                ret.w = v[3];
            }
            else if (v.size() == 3)
            {
                ret.x = v[0];
                ret.y = v[1];
                ret.z = v[2];
            }
            else if (v.size() == 2)
            {
                ret.x = v[0];
                ret.y = v[1];
            }
            else if (v.size() == 1)
            {
                ret.x = v[0];
                ret.y = v[0];
                ret.z = v[0];
                ret.w = v[0];
            }
        }
    }
    return ret;
}
template <typename T>
inline T toml_get(const toml::table& table, const char* pszName, const T& val = T{})
{
    auto itr = table.find(pszName);
    if (itr != table.end())
    {
        return toml::get<T>(itr->second);
    }
    return val;
}

inline const toml::table toml_get_table(const toml::table& table, const char* child)
{
    auto itr = table.find(child);
    if (itr != table.end())
    {
        if (itr->second.is_table())
        {
            return itr->second.as_table();
        }
    }
    return toml::table{};
}

template <typename T>
inline void toml_set(toml::table& table, const char* pszName, const T& val = T{})
{
    auto itr = table.find(pszName);
    if (itr != table.end())
    {
        //return toml::set<T>(itr->second, val);
        assert(!"Implement me!");
    }
    table[pszName] = val;
}

inline int toml_line(const toml::table& table, const char* pszName)
{
    auto itr = table.find(pszName);
    if (itr != table.end())
    {
        return itr->second.location().line() - 1;
    }
    return 0;
}

}
