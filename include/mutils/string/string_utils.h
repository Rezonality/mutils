#pragma once

#include <functional>
#include <string>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace MUtils
{

inline size_t string_count_utf8_bytes(unsigned int c)
{
    if (c < 0x80)
        return 1;
    if (c < 0x800)
        return 2;
    if (c >= 0xdc00 && c < 0xe000)
        return 0;
    if (c >= 0xd800 && c < 0xdc00)
        return 4;
    return 3;
}

inline size_t string_utf8_length(const char* s)
{
    size_t stringLength = 0;
    while (*s != 0)
    {
        size_t len = 1;
        while (len <= 4 && *s)
        {
            if ((*s++ & 0xc0) != 0x80)
                break;
            len++;
        }
        stringLength += len;
    }
    return stringLength;
}

std::string string_replace(std::string subject, const std::string& search, const std::string& replace);
void string_replace_in_place(std::string& subject, const std::string& search, const std::string& replace);

// trim from beginning of string (left)
inline std::string string_left_trim(std::string s, const char* t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from end of string (right)
inline std::string string_right_trim(std::string s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from both ends of string (left & right)
inline std::string string_trim(std::string s, const char* t = " \t\n\r\f\v")
{
    return string_left_trim(string_right_trim(s, t), t);
}

template <typename T>
std::string string_from_value(const T& t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

template <typename T>
T string_to_value(const std::string& s)
{
    std::istringstream stream(s);
    T t;
    stream >> t;
    return t;
}

inline std::wstring string_make_wstring(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

std::string string_from_wstring(const std::wstring& str);
std::string string_tolower(const std::string& str);

struct StringId
{
    uint32_t id = 0;
    StringId()
    {
    }
    StringId(const char* pszString);
    StringId(const std::string& str);
    StringId(uint32_t _id)
    {
        id = _id;
    }

    bool operator==(const StringId& rhs) const
    {
        return id == rhs.id;
    }
    const StringId& operator=(const char* pszString);
    const StringId& operator=(const std::string& str);
    bool operator<(const StringId& rhs) const
    {
        return id < rhs.id;
    }

    std::string ToString() const
    {
        auto itr = stringLookup.find(id);
        if (itr == stringLookup.end())
        {
            return "murmur:" + std::to_string(id);
        }
        return itr->second;
    }

    static std::unordered_map<uint32_t, std::string> stringLookup;
};

inline std::ostream& operator<<(std::ostream& str, StringId id)
{
    str << id.ToString();
    return str;
}

std::string string_url_encode(const std::string& value);
void string_split(const std::string& text, const char* delims, std::vector<std::string>& tokens);
std::vector<std::string> string_split(const std::string& text, const char* delims);
void string_split_lines(const std::string& text, std::vector<std::string>& tokens);
std::vector<std::string> string_split_lines(const std::string& text);
std::vector<std::string> string_split_delim_string(const std::string& str, const std::string& delim);
void string_split_each(const std::string& text, const char* delims, std::function<bool(size_t, size_t)> fn);
void string_split_each(char* text, size_t start, size_t end, const char* delims, std::function<bool(size_t, size_t)> fn);
size_t string_first_of(const char* text, size_t start, size_t end, const char* delims);
size_t string_first_not_of(const char* text, size_t start, size_t end, const char* delims);
std::pair<uint32_t, uint32_t> string_convert_index_to_line_offset(const std::string& str, uint32_t index);

inline bool string_equals(const std::string& str, const std::string& str2)
{
    return str == str2;
}
} // namespace MUtils

namespace std
{
template <>
struct hash<MUtils::StringId>
{
    std::size_t operator()(const MUtils::StringId& k) const
    {
        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:

        return std::hash<uint32_t>()(k.id);
    }
};
} // namespace std

namespace MUtils
{
inline bool string_equals(const StringId lhs, const StringId rhs)
{
    return lhs.id == rhs.id;
}
} // namespace MUtils
