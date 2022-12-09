#include <algorithm>
#include <cassert>
#include <cctype>
#include <codecvt>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#include "mutils/string/string_utils.h"

using namespace std;

// StringUtils.
// Note, simple, effective string utilities which concentrate on useful functinality and correctness and not on speed!
namespace MUtils
{

// https://stackoverflow.com/a/17708801/18942
string string_url_encode(const string& value)
{
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
    {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char)c);
        escaped << nouppercase;
    }

    return escaped.str();
}

std::string string_tolower(const std::string& str)
{
    std::string copy = str;
    std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
    return copy;
}

std::string string_replace(std::string subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

void string_replace_in_place(std::string& subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

#pragma warning(disable : 4996)
//https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
std::string string_from_wstring(const std::wstring& str)
{
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    std::string converted_str = converter.to_bytes(str);
    return converted_str;
}
#pragma warning(default : 4996)

// CM: I can't remember where this came from; please let me know if you do!
// I know it is open source, but not sure who wrote it.
uint32_t murmur_hash(const void* key, int len, uint32_t seed)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value
    unsigned int h = seed ^ len;

    // Mix 4 bytes at a time into the hash
    const unsigned char* data = (const unsigned char*)key;

    while (len >= 4)
    {
#ifdef PLATFORM_BIG_ENDIAN
        unsigned int k = (data[0]) + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
#else
        unsigned int k = *(unsigned int*)data;
#endif

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch (len)
    {
    case 3:
        h ^= data[2] << 16;
    case 2:
        h ^= data[1] << 8;
    case 1:
        h ^= data[0];
        h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

/// Inverts a (h ^= h >> s) operation with 8 <= s <= 16
unsigned int invert_shift_xor(unsigned int hs, unsigned int s)
{
    assert(s >= 8 && s <= 16);
    unsigned hs0 = hs >> 24;
    unsigned hs1 = (hs >> 16) & 0xff;
    unsigned hs2 = (hs >> 8) & 0xff;
    unsigned hs3 = hs & 0xff;

    unsigned h0 = hs0;
    unsigned h1 = hs1 ^ (h0 >> (s - 8));
    unsigned h2 = (hs2 ^ (h0 << (16 - s)) ^ (h1 >> (s - 8))) & 0xff;
    unsigned h3 = (hs3 ^ (h1 << (16 - s)) ^ (h2 >> (s - 8))) & 0xff;
    return (h0 << 24) + (h1 << 16) + (h2 << 8) + h3;
}

unsigned int murmur_hash_inverse(unsigned int h, unsigned int seed)
{
    const unsigned int m = 0x5bd1e995;
    const unsigned int minv = 0xe59b19bd; // Multiplicative inverse of m under % 2^32
    const int r = 24;

    h = invert_shift_xor(h, 15);
    h *= minv;
    h = invert_shift_xor(h, 13);

    unsigned int hforward = seed ^ 4;
    hforward *= m;
    unsigned int k = hforward ^ h;
    k *= minv;
    k ^= k >> r;
    k *= minv;

#ifdef PLATFORM_BIG_ENDIAN
    char* data = (char*)&k;
    k = (data[0]) + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
#endif

    return k;
}

uint64_t murmur_hash_64(const void* key, uint32_t len, uint64_t seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const uint32_t r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t* data = (const uint64_t*)key;
    const uint64_t* end = data + (len / 8);

    while (data != end)
    {
#ifdef PLATFORM_BIG_ENDIAN
        uint64 k = *data++;
        char* p = (char*)&k;
        char c;
        c = p[0];
        p[0] = p[7];
        p[7] = c;
        c = p[1];
        p[1] = p[6];
        p[6] = c;
        c = p[2];
        p[2] = p[5];
        p[5] = c;
        c = p[3];
        p[3] = p[4];
        p[4] = c;
#else
        uint64_t k = *data++;
#endif

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char* data2 = (const unsigned char*)data;

    switch (len & 7)
    {
    case 7:
        h ^= uint64_t(data2[6]) << 48;
    case 6:
        h ^= uint64_t(data2[5]) << 40;
    case 5:
        h ^= uint64_t(data2[4]) << 32;
    case 4:
        h ^= uint64_t(data2[3]) << 24;
    case 3:
        h ^= uint64_t(data2[2]) << 16;
    case 2:
        h ^= uint64_t(data2[1]) << 8;
    case 1:
        h ^= uint64_t(data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

/*
//https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
std::string makeStr(const std::wstring& str)
{
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    std::string converted_str = converter.to_bytes(str);
    return converted_str;
}
*/

std::vector<std::string> string_split(const std::string& text, const char* delims)
{
    std::vector<std::string> tok;
    string_split(text, delims, tok);
    return tok;
}

// String split with multiple delims
// https://stackoverflow.com/a/7408245/18942
void string_split(const std::string& text, const char* delims, std::vector<std::string>& tokens)
{
    tokens.clear();
    std::size_t start = text.find_first_not_of(delims), end = 0;

    while ((end = text.find_first_of(delims, start)) != std::string::npos)
    {
        tokens.push_back(text.substr(start, end - start));
        start = text.find_first_not_of(delims, end);
    }
    if (start != std::string::npos)
        tokens.push_back(text.substr(start));
}

void string_split_each(const std::string& text, const char* delims, std::function<bool(size_t, size_t)> fn)
{
    std::size_t start = text.find_first_not_of(delims), end = 0;

    while ((end = text.find_first_of(delims, start)) != std::string::npos)
    {
        if (!fn(start, end - start))
            return;
        start = text.find_first_not_of(delims, end);
    }
    if (start != std::string::npos)
        fn(start, text.length() - start);
}

std::vector<std::string> string_split_delim_string(const std::string& str, const std::string& delim)
{
    std::vector<std::string> ret;
    std::size_t current, previous = 0;
    current = str.find(delim);
    while (current != std::string::npos)
    {
        ret.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find(delim, previous);
    }
    ret.push_back(str.substr(previous, current - previous));
    return ret;
}

size_t string_first_not_of(const char* text, size_t start, size_t end, const char* delims)
{
    for (auto index = start; index < end; index++)
    {
        bool found = false;
        auto pDelim = delims;
        while (*pDelim != 0)
        {
            if (text[index] == *pDelim++)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return index;
    }
    return std::string::npos;
}

size_t string_first_of(const char* text, size_t start, size_t end, const char* delims)
{
    for (auto index = start; index < end; index++)
    {
        auto pDelim = delims;
        while (*pDelim != 0)
        {
            if (text[index] == *pDelim++)
            {
                return index;
            }
        }
    }
    return std::string::npos;
}

void string_split_each(char* text, size_t startIndex, size_t endIndex, const char* delims, std::function<bool(size_t, size_t)> fn)
{
    // Skip delims (start now at first thing that is not a delim)
    std::size_t start = string_first_not_of(text, startIndex, endIndex, delims);
    std::size_t end;

    // Find first delim (end now at first delim)
    while ((end = string_first_of(text, start, endIndex, delims)) != std::string::npos)
    {
        // Callback with string between delims
        if (!fn(start, end))
            return;

        if (text[end] == 0 && end < endIndex)
            end++;

        // Find the first non-delim
        start = string_first_not_of(text, end, endIndex, delims);
    }
    // Return the last one
    if (start != std::string::npos)
        fn(start, endIndex);
}

// Given an index into a string, count the newlines and return (line, lineOffset).
// I wrote this with a fuzzy head due to accidental caffeine intake.  It worked first time, but I should write
// a unit test.  It is probably inelegant.
std::pair<uint32_t, uint32_t> string_convert_index_to_line_offset(const std::string& str, uint32_t index)
{
    uint32_t line = 0;
    std::pair<uint32_t, uint32_t> lineRange = std::make_pair(0, 0);

    uint32_t currentIndex = 0;
    auto itr = str.begin();
    while (itr != str.end())
    {
        lineRange.second = currentIndex;

        // Found it
        if (index >= lineRange.first && index <= lineRange.second)
        {
            return std::make_pair(line, index - lineRange.first);
        }

        // Find line end
        if (*itr == '\r' || *itr == '\n')
        {
            bool isR = (*itr == '\r');
            // Skip
            itr++;
            currentIndex++;

            // Not at end and a '\r\n'
            if (itr != str.end() && isR && *itr == '\n')
            {
                // Skip another
                itr++;
                currentIndex++;
            }

            // New line starts here
            line++;
            lineRange.first = (uint32_t)currentIndex;

            if (index >= currentIndex)
            {
                return std::make_pair(line, index - lineRange.first);
            }
            continue;
        }
        itr++;
        currentIndex++;
    }
    return std::make_pair(line, index - lineRange.first);
}

void string_split_lines(const std::string& text, std::vector<std::string>& split)
{
    string_split(text, "\r\n", split);
}

std::vector<std::string> string_split_lines(const std::string& text)
{
    std::vector<std::string> split;
    string_split(text, "\r\n", split);
    return split;
}

StringId::StringId(const char* pszString)
{
    id = murmur_hash(pszString, (int)strlen(pszString), 0);
    LookupInstance()[id] = pszString;
}

StringId::StringId(const std::string& str)
{
    id = murmur_hash(str.c_str(), (int)str.length(), 0);
    LookupInstance()[id] = str;
}

const StringId& StringId::operator=(const char* pszString)
{
    id = murmur_hash(pszString, (int)strlen(pszString), 0);
    LookupInstance()[id] = pszString;
    return *this;
}

const StringId& StringId::operator=(const std::string& str)
{
    id = murmur_hash(str.c_str(), (int)str.length(), 0);
    LookupInstance()[id] = str;
    return *this;
}

std::vector<std::vector<std::string>> string_get_string_grid(const std::string& str)
{
    // Gather into array
    std::vector<std::string> lines;
    string_split_lines(str, lines);
    std::vector<std::vector<std::string>> arrayLines;
    for (auto& line : lines)
    {
        std::vector<std::string> vals = string_split(line, "\t, ");
        if (!vals.empty())
        {
            arrayLines.push_back(vals);
        }
    }
    return arrayLines;
}

std::vector<std::vector<int>> string_get_integer_grid(const std::string& str, const std::string& delim)
{
    // Gather into array
    std::vector<std::string> lines;
    string_split_lines(str, lines);
    std::vector<std::vector<int>> arrayLines;
    for (auto& line : lines)
    {
        std::vector<int> vals;
        if (!delim.empty())
        {
            auto strVals = string_split(line, delim.c_str());
            std::transform(strVals.begin(), strVals.end(), back_inserter(vals), [](const std::string& str) { return stoi(str); });
        }
        else
        {
            std::transform(line.begin(), line.end(), back_inserter(vals), [](const char& str) { return str - '0'; });
        }
        if (!vals.empty())
        {
            arrayLines.push_back(vals);
        }
    }
    return arrayLines;
}

std::vector<int> string_get_integers(const std::string& str)
{
    std::vector<int> vals;
    auto strVals = string_split(str, "\t\n\r ,");
    std::transform(strVals.begin(), strVals.end(), back_inserter(vals), [](const std::string& str) { return stoi(str); });
    return vals;
}

} // namespace MUtils
