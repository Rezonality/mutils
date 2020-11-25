#include "mutils/compile/compile_messages.h"
#include "mutils/string/string_utils.h"

namespace MUtils
{

// We need to filter:
// <filepath>(<linenum>,<column>-?<column>): message
// This can probably done efficiently with regex, but I'm no expert on regex,
// and it's easy to miss thing. So here we just do simple string searches
// It works, and makes up an error when it doesn't, so I can fix it!
void compile_parse_shader_errors(gsl::not_null<CompileResult*> pResult, const std::string& messages)
{
    // Try to parse the DX error string into file, line, column and message
    // Exception should catch silly mistakes.
    std::vector<std::string> errors;
    string_split(messages, "\n", errors);

    std::ostringstream strUnknown;

    for (auto error : errors)
    {
        auto pMsg = std::make_shared<CompileMessage>();
        pMsg->filePath = pResult->fileSource.string();
        pMsg->rawText = error;

        auto lower = string_tolower(error);
        if (lower.find("warning") != std::string::npos)
        {
            pMsg->msgType = CompileMessageType::Warning;
        }
        else
        {
            pMsg->msgType = CompileMessageType::Error;
        }

        try
        {
            auto bracketPos = error.find_first_of('(');
            if (bracketPos != std::string::npos)
            {
                auto lastBracket = error.find(")", bracketPos);
                if (lastBracket)
                {
                    // file (line, colstart-colend) error
                    pMsg->filePath = string_trim(error.substr(0, bracketPos));
                    pMsg->text = string_trim(error.substr(lastBracket, error.size() - lastBracket), " \t():\r\n\f\v");

                    // Handle special multiline message; where there is preamble before error info
                    if (!strUnknown.str().empty())
                    {
                        pMsg->text = strUnknown.str() + pMsg->text;
               
                        // Try to extract warning/error context from the whole string
                        // <rant>Drivers are all different, and the API/Driver is not designed to report useful errors to tools.
                        // What I wouldn't give for a simple line number/offset API for errors....
                        // OpenGL is even inconsistent in the formatting of its errors.  DX is better in that regard. </rant>
                        auto str = string_tolower(pMsg->text);
                        if (str.find("info") != std::string::npos ||
                            str.find("warning") != std::string::npos)
                        {
                            pMsg->msgType = CompileMessageType::Warning;
                        }
                        else if (str.find("error") != std::string::npos)
                        {
                            pMsg->msgType = CompileMessageType::Error;
                        }

                        strUnknown = std::ostringstream();
                    }

                    std::string numbers = string_trim(error.substr(bracketPos, lastBracket - bracketPos), "( )");
                    auto numVec = string_split(numbers, ",");

                    // (3, 5 - 3)
                    if (!numVec.empty())
                    {
                        pMsg->line = std::max(0, std::stoi(numVec[0]) - 1);
                    }
                    if (numVec.size() > 1)
                    {
                        auto columnVec = string_split(numVec[1], "-");
                        if (!columnVec.empty())
                        {
                            pMsg->range.first = std::max(0, std::stoi(columnVec[0]) - 1);
                            if (columnVec.size() > 1)
                            {
                                pMsg->range.second = std::stoi(columnVec[1]);
                            }
                            else
                            {
                                pMsg->range.second = pMsg->range.first + 1;
                            }
                        }
                    }
                }
            }
            else
            {
                strUnknown << pMsg->rawText << std::endl;
                continue;
            }
        }
        catch (...)
        {
            pMsg->text = "Failed to parse compiler error:\n" + error;
            pMsg->line = 0;
            pMsg->msgType = CompileMessageType::Error;
        }

        pResult->messages.push_back(pMsg);
    }

    if (pResult->messages.empty())
    {
        auto pMsg = std::make_shared<CompileMessage>();
        pMsg->filePath = pResult->fileSource.string();
        pMsg->rawText = messages;
        pMsg->msgType = CompileMessageType::Error;
        if (!strUnknown.str().empty())
        {
            pMsg->text = strUnknown.str();
        }
        else
        {
            pMsg->text = messages;
        }
        pResult->messages.push_back(pMsg);
    }
}

} // MUtils
