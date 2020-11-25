#include "mutils/algorithm/container_utils.h"
#include "mutils/string/string_utils.h"
#include "mutils/logger/logger.h"
#include "mutils/compile/meta_tags.h"

#include <functional>
#include <vector>
#include <string>

namespace MUtils
{

MetaTags_t parse_meta_tags(const std::string& text)
{
    MetaTags_t tags;
    try
    {
        std::vector<std::string> lines;
        string_split_lines(text, lines);

        int32_t lineNumber = 0;
        for (auto& line : lines)
        {
            auto pos = line.find("//", 0);
            if (pos != std::string::npos)
            {
                std::vector<std::string> tokens;
                string_split(line.substr(pos + 2), " \t,", tokens);

                // Always lower case
                for (int tok = 0; tok < tokens.size(); tok++)
                {
                    tokens[tok] = string_tolower(tokens[tok]);
                }

                if (!tokens.empty() && !tokens[0].empty() && tokens[0][0] == '#')
                {
                    if (tokens.size() == 1)
                    {
                        tags[tokens[0]] = TagValue{ "", lineNumber };
                    }
                    else if (tokens.size() == 2)
                    { 
                        tags[tokens[0]] = TagValue{ tokens[1], lineNumber };
                    }
                }
            }
            lineNumber++;
        }
    }
    catch (std::exception & ex)
    {
        (void)&ex;
        LOG(DBG, ex.what());
    }
    return tags;
}

}; // namespace Jorvik
