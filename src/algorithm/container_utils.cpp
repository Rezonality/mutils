#include "mutils/string/string_utils.h"
#include "mutils/algorithm/container_utils.h"

#include <algorithm>
#include <chrono>

namespace MUtils
{

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

std::vector<std::vector<int>> string_get_integer_grid(const std::string& str)
{
    // Gather into array
    std::vector<std::string> lines;
    string_split_lines(str, lines);
    std::vector<std::vector<int>> arrayLines;
    for (auto& line : lines)
    {
        std::vector<int> vals;
        auto strVals = string_split(line, "\t ");
        std::transform(strVals.begin(), strVals.end(), back_inserter(vals), [](const std::string& str) { return stoi(str); });
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

} // MUtils
