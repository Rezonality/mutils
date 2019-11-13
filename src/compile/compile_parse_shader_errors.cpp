#include "mutils/compile/compile_parse_shader_errors.h"

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
    for (auto error : errors)
    {
        auto pMsg = std::make_shared<CompileMessage>();
        pMsg->filePath = spResult->fileSource.string();
        pMsg->rawText = error;

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
                pMsg->text = error;
                pMsg->line = 0;
                pMsg->range = std::make_pair(0, 0);
                pMsg->msgType = CompileMessageType::Error;
            }
        }
        catch (...)
        {
            pMsg->text = "Failed to parse compiler error:\n" + error;
            pMsg->line = 0;
            pMsg->msgType = CompileMessageType::Error;
        }
        spResult->messages.push_back(pMsg);
    }
}
