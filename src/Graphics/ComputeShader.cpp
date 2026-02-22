#include "Graphics/ComputeShader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

ComputeShader::~ComputeShader()
{
    if (m_Program)
    {
        glDeleteProgram(m_Program);
    }
}

ComputeShader::ComputeShader(ComputeShader &&other) noexcept
    : m_Program(other.m_Program), m_ComputePath(std::move(other.m_ComputePath)), m_LastError(std::move(other.m_LastError)), m_UniformCache(std::move(other.m_UniformCache))
{
    other.m_Program = 0;
}

ComputeShader &ComputeShader::operator=(ComputeShader &&other) noexcept
{
    if (this != &other)
    {
        if (m_Program)
        {
            glDeleteProgram(m_Program);
        }
        m_Program = other.m_Program;
        m_ComputePath = std::move(other.m_ComputePath);
        m_LastError = std::move(other.m_LastError);
        m_UniformCache = std::move(other.m_UniformCache);
        other.m_Program = 0;
    }
    return *this;
}

bool ComputeShader::LoadFromFile(const std::string &computePath)
{
    m_ComputePath = computePath;
    return Reload();
}

bool ComputeShader::Reload()
{
    m_LastError.clear();
    m_UniformCache.clear();

    std::string source = ReadFile(m_ComputePath);
    if (source.empty())
    {
        m_LastError = "Failed to read compute shader: " + m_ComputePath;
        return false;
    }

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        m_LastError.resize(length);
        glGetShaderInfoLog(shader, length, nullptr, m_LastError.data());
        m_LastError = "Compute shader error:\n" + m_LastError;
        glDeleteShader(shader);
        return false;
    }

    GLuint newProgram = glCreateProgram();
    glAttachShader(newProgram, shader);
    glLinkProgram(newProgram);
    glDeleteShader(shader);

    glGetProgramiv(newProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint length;
        glGetProgramiv(newProgram, GL_INFO_LOG_LENGTH, &length);
        m_LastError.resize(length);
        glGetProgramInfoLog(newProgram, length, nullptr, m_LastError.data());
        m_LastError = "Compute shader linking error:\n" + m_LastError;
        glDeleteProgram(newProgram);
        return false;
    }

    if (m_Program)
    {
        glDeleteProgram(m_Program);
    }
    m_Program = newProgram;

    std::cout << "[ComputeShader] Loaded: " << m_ComputePath << std::endl;
    return true;
}

void ComputeShader::Use() const
{
    glUseProgram(m_Program);
}

void ComputeShader::Dispatch(GLuint numGroupsX, GLuint numGroupsY, GLuint numGroupsZ) const
{
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}

GLint ComputeShader::GetUniformLocation(const std::string &name)
{
    auto it = m_UniformCache.find(name);
    if (it != m_UniformCache.end())
    {
        return it->second;
    }
    GLint location = glGetUniformLocation(m_Program, name.c_str());
    m_UniformCache[name] = location;
    return location;
}

void ComputeShader::SetInt(const std::string &name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void ComputeShader::SetFloat(const std::string &name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void ComputeShader::SetVec2(const std::string &name, const glm::vec2 &value)
{
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::SetVec3(const std::string &name, const glm::vec3 &value)
{
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::SetVec4(const std::string &name, const glm::vec4 &value)
{
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void ComputeShader::SetMat3(const std::string &name, const glm::mat3 &value)
{
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ComputeShader::SetMat4(const std::string &name, const glm::mat4 &value)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

std::string ComputeShader::ReadFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file)
    {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
