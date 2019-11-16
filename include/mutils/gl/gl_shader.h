#pragma once

#include <GL/gl3w.h>

#include "mutils/file/file.h"
#include "mutils/shader/shader.h"

namespace MUtils
{

struct GLCompileResult : CompileResult
{
    GLuint Id;
};

std::shared_ptr<GLCompileResult> gl_compile_shader(ShaderType type, const ShaderPackage& package);
std::shared_ptr<GLCompileResult> gl_load_program(const fs::path& vertex_file_path, const fs::path& fragment_file_path);
std::shared_ptr<GLCompileResult> gl_link_shaders(std::shared_ptr<GLCompileResult> spVertex, std::shared_ptr<GLCompileResult> spGeometry, std::shared_ptr<GLCompileResult> spPixel);

void gl_delete_program(std::shared_ptr<GLCompileResult> spResult);
void gl_delete_shader(std::shared_ptr<GLCompileResult> spResult);

void gl_parse_messages(std::shared_ptr<GLCompileResult>& spResult, const std::string& messages);

}

