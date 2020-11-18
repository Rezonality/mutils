#pragma once

#include <cstdint>
#include <exception>
#include <nlohmann/json.hpp>
#include <magic_enum.hpp>

namespace MUtils
{

template <class T>
T json_enum_cast(const nlohmann::json& val, T def)
{
    if (val.is_null())
    {
        return def;
    }
    auto type = magic_enum::enum_cast<T>((std::string)val);
    if (type.has_value())
    {
        return type.value();
    }
    return def;
}

std::string json_to_string(const nlohmann::json& val, const char* name, const char* def = "")
{
    if (val.count(name) != 0)
    {
        if (val.at(name).is_string())
        {
            return val.at(name).get<std::string>();
        }
    }
    return def;
}

int json_to_int(const nlohmann::json& val, const char* name, int def = 0)
{
    if (val.count(name) != 0)
    {
        if (val.at(name).is_number_integer())
        {
            return val.at(name).get<int>();
        }
    }
    return def;
}

bool json_string_to_bool(const nlohmann::json& val, const char* name, bool def = false)
{
    auto str = json_to_string(val, name);
    if (str == "true")
    {
        return true;
    }
    return false;
}

uint64_t json_to_date(const nlohmann::json& val, const char* name, uint64_t def = 0)
{
    std::string dt = json_to_string(val, name, "");
    if (dt.empty())
    {
        return 0;
    }
    try
    {
        return std::stoull(dt);
    }
    catch (std::exception&)
    {
        return def;
    }
    return def;
}

} // json
