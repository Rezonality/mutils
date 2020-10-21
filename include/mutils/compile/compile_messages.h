#pragma once

#include <vector>
#include <memory>
#include "mutils/file/file.h"
#include "mutils/compile/meta_tags.h"
#include <gsl-lite/gsl-lite.hpp>

namespace MUtils
{

// Messages during compile steps
enum class CompileMessageType
{
    Warning,
    Error,
    Info
};

struct CompileMessage
{
    CompileMessage() {};
    CompileMessage(CompileMessageType type, const fs::path& path)
        :filePath(path),
        text("Unknown issue compiling: " + path.string())
    {
    }

    CompileMessage(CompileMessageType type, const fs::path& path, const std::string& message)
        :filePath(path),
        text(message)
    {
    }

    CompileMessage(CompileMessageType type, const fs::path& path, const std::string& message, int line)
        :filePath(path),
        text(message),
        line(line)
    {
    }

    // Text before it is parsed
    std::string rawText;

    // Parsed message, file, line, buffer range and type
    std::string text;
    fs::path filePath;
    int32_t line = 0;
    uint32_t fragmentIndex = 0;
    std::pair<int32_t, int32_t> range = std::make_pair(-1, -1);
    CompileMessageType msgType = CompileMessageType::Error;
};

enum class CompileState
{
    Invalid,
    Valid
};

struct CompileResult
{
    fs::path fileSource;
    std::vector<std::shared_ptr<CompileMessage>> messages;
    CompileState state = CompileState::Invalid;
    MetaTags_t tags;
};

void compile_parse_shader_errors(gsl::not_null<CompileResult*> pResult, const std::string& messages);

} // MUtils
