#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>

#include <stdlib.h>
#include <string.h>

#include <GL/gl3w.h>

#include <mutils/logger/logger.h>
#include <mutils/gl/gl_shader.h>
#include <mutils/string/stringutils.h>
#include <mutils/compile/compile_messages.h>

#undef min
#undef max

using namespace std;

namespace MUtils
{

std::shared_ptr<GLCompileResult> gl_compile_shader(ShaderType type, const fs::path& file_path, const std::string& shaderCode)
{
    auto spResult = std::make_shared<GLCompileResult>();
    spResult->fileSource = file_path;
    spResult->Id = 0;

    try
    {
        switch (type)
        {
        case ShaderType::Vertex:
        {
            spResult->Id = glCreateShader(GL_VERTEX_SHADER);
        }
        break;
        case ShaderType::Geometry:
        {
            spResult->Id = glCreateShader(GL_GEOMETRY_SHADER);
        }
        break;
        case ShaderType::Pixel:
        {
            spResult->Id = glCreateShader(GL_FRAGMENT_SHADER);
        }
        break;
        case ShaderType::Unknown:
        {
            spResult->messages.push_back(std::make_shared<CompileMessage>(CompileMessageType::Error, "Shader type could not be determined.  Missing #shadertype tag?"));
            return spResult;
        }
        break;
        default:
        {
            spResult->messages.push_back(std::make_shared<CompileMessage>(CompileMessageType::Error, "Shader type not supported yet"));
            return spResult;
        }
        break;
        }

        std::string shader;
        if (shaderCode.empty())
        {
            std::ifstream shaderStream(file_path, std::ios::in);
            if (shaderStream.is_open())
            {
                std::stringstream sstr;
                sstr << shaderStream.rdbuf();
                shader = sstr.str();
                shaderStream.close();
            }
            else
            {
                spResult->messages.push_back(std::make_shared<CompileMessage>(CompileMessageType::Error, file_path, "Couldn't open file"));
                return spResult;
            }
        }
        else
        {
            shader = shaderCode;
        }

        GLint Result = GL_FALSE;
        int InfoLogLength;

        // Compile Fragment Shader
        char const* sourcePointer = shader.c_str();
        glShaderSource(spResult->Id, 1, &sourcePointer, NULL);
        glCompileShader(spResult->Id);

        // Check Fragment Shader
        glGetShaderiv(spResult->Id, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(spResult->Id, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0)
        {
            std::vector<char> errorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(spResult->Id, InfoLogLength, NULL, &errorMessage[0]);
            compile_parse_shader_errors(spResult.get(), &errorMessage[0]);
        }

        if (Result == GL_FALSE)
        {
            glDeleteShader(spResult->Id);
            spResult->Id = 0;
        }
        spResult->state = (Result == GL_TRUE) ? CompileState::Valid : CompileState::Invalid;
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << "Exception compiling shader: " << ex.what();
    }
    return spResult;
}

std::shared_ptr<GLCompileResult> gl_link_shaders(std::shared_ptr<GLCompileResult> spVertex, std::shared_ptr<GLCompileResult> spGeometry, std::shared_ptr<GLCompileResult> spPixel)
{
    if (spVertex->state == CompileState::Invalid)
    {
        return spVertex;
    }

    if (spPixel->state == CompileState::Invalid)
    {
        return spPixel;
    }

    // Geometry allowed to be null
    if (spGeometry != nullptr && spGeometry->state == CompileState::Invalid)
    {
        return spGeometry;
    }

    auto spResult = std::make_shared<GLCompileResult>();
    spResult->fileSource = spVertex->fileSource;
    spResult->Id = 0;

    // Link the program
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, spVertex->Id);
    glAttachShader(ProgramID, spPixel->Id);
    if (spGeometry)
    {
        glAttachShader(ProgramID, spGeometry->Id);
    }
    glLinkProgram(ProgramID);

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        compile_parse_shader_errors(spResult.get(), &ProgramErrorMessage[0]);
        for (auto& msg : spResult->messages)
        {
            // Try to figure out the error source in the link step!
            auto err = string_tolower(msg->rawText);
            if (err.find("vertex") != std::string::npos)
            {
                spVertex->messages.push_back(msg);
            }
            else if (err.find("pixel") != std::string::npos)
            {
                spPixel->messages.push_back(msg);
            }
        }
    }

    if (Result == GL_TRUE)
    {
        spResult->Id = ProgramID;
    }
    else
    {
        glDetachShader(ProgramID, spVertex->Id);
        glDetachShader(ProgramID, spPixel->Id);
        if (spGeometry != nullptr)
        {
            glDetachShader(ProgramID, spGeometry->Id);
        }
        glDeleteProgram(ProgramID);
        glGetError();
    }

    spResult->state = (Result == GL_TRUE) ? CompileState::Valid : CompileState::Invalid;
    return spResult;
}

std::shared_ptr<GLCompileResult> gl_load_program(const fs::path& vertex_file_path, const fs::path& fragment_file_path)
{
    auto pVertex = gl_compile_shader(ShaderType::Vertex, vertex_file_path);
    auto pFragment = gl_compile_shader(ShaderType::Pixel, fragment_file_path);

    if (pVertex->state == CompileState::Invalid)
    {
        return pVertex;
    }

    if (pFragment->state == CompileState::Invalid)
    {
        return pFragment;
    }

    auto pRet = gl_link_shaders(pVertex, nullptr, pFragment);

    glDetachShader(pRet->Id, pVertex->Id);
    glDetachShader(pRet->Id, pFragment->Id);

    glDeleteShader(pVertex->Id);
    glDeleteShader(pFragment->Id);
    return pRet;
}

void gl_delete_shader(std::shared_ptr<GLCompileResult> spResult)
{
    if (spResult->state == CompileState::Valid)
    {
        glDeleteShader(spResult->Id);
    }
}

void gl_delete_program(std::shared_ptr<GLCompileResult> spResult)
{
    if (spResult->state == CompileState::Valid)
    {
        glDeleteProgram(spResult->Id);
    }
}

} // MUtils
