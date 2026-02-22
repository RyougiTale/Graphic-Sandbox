#include "Graphics/Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include "Util/GLog.h"

Shader::~Shader()
{
    if (m_Program)
    {
        glDeleteProgram(m_Program);
    }
}

Shader::Shader(Shader &&other) noexcept
    : m_Program(other.m_Program),
      m_ShaderPaths(std::move(other.m_ShaderPaths)),
      m_LastError(std::move(other.m_LastError)),
      m_UniformCache(std::move(other.m_UniformCache))
{
    other.m_Program = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept
{
    if (this != &other)
    {
        if (m_Program)
        {
            glDeleteProgram(m_Program);
        }
        m_Program = other.m_Program;
        m_ShaderPaths = std::move(other.m_ShaderPaths);
        m_LastError = std::move(other.m_LastError);
        m_UniformCache = std::move(other.m_UniformCache);
        other.m_Program = 0;
    }
    return *this;
}

bool Shader::LoadFromFiles(const std::string &vertexPath, const std::string &fragmentPath)
{
    m_ShaderPaths.clear();
    m_ShaderPaths[GL_VERTEX_SHADER] = vertexPath;
    m_ShaderPaths[GL_FRAGMENT_SHADER] = fragmentPath;
    return Reload();
}

bool Shader::LoadFromFiles(std::initializer_list<std::pair<GLenum, std::string>> stages)
{
    m_ShaderPaths.clear();
    for (auto &[type, path] : stages)
    {
        m_ShaderPaths[type] = path;
    }
    return Reload();
}

bool Shader::Reload()
{
    m_LastError.clear();
    m_UniformCache.clear();

    std::vector<GLuint> compiledShaders;

    for (auto &[type, path] : m_ShaderPaths)
    {
        std::string source = ReadFile(path);
        if (source.empty())
        {
            m_LastError = "Failed to read " + std::string(ShaderTypeName(type)) + " shader: " + path;
            LOG_WARN(m_LastError);
            for (GLuint s : compiledShaders)
                glDeleteShader(s);
            return false;
        }

        GLuint shader = CompileShader(type, source, m_LastError);
        if (!shader)
        {
            m_LastError = std::string(ShaderTypeName(type)) + " shader error:\n" + m_LastError;
            LOG_WARN(m_LastError);
            for (GLuint s : compiledShaders)
                glDeleteShader(s);
            return false;
        }

        compiledShaders.push_back(shader);
    }

    GLuint newProgram = LinkProgram(compiledShaders, m_LastError);
    for (GLuint s : compiledShaders)
        glDeleteShader(s);

    if (!newProgram)
    {
        m_LastError = "Linking error:\n" + m_LastError;
        LOG_WARN(m_LastError);
        return false;
    }

    if (m_Program)
    {
        glDeleteProgram(m_Program);
    }
    m_Program = newProgram;

    for (auto &[type, path] : m_ShaderPaths)
    {
        LOG_INFO("[Shader] Loaded: ", path);
    }
    return true;
}

void Shader::Use() const
{
    glUseProgram(m_Program);
}

// 封装glGetUniformLocation
GLint Shader::GetUniformLocation(const std::string &name)
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

void Shader::SetInt(const std::string &name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetFloat(const std::string &name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVec2(const std::string &name, const glm::vec2 &value)
{
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string &name, const glm::vec3 &value)
{
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetVec4(const std::string &name, const glm::vec4 &value)
{
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetMat3(const std::string &name, const glm::mat3 &value)
{
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string &name, const glm::mat4 &value)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

std::string Shader::ReadFile(const std::string &path)
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

GLuint Shader::CompileShader(GLenum type, const std::string &source, std::string &errorOut)
{
    GLuint shader = glCreateShader(type);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        errorOut.resize(length);
        glGetShaderInfoLog(shader, length, nullptr, errorOut.data());
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint Shader::LinkProgram(const std::vector<GLuint> &shaders, std::string &errorOut)
{
    GLuint program = glCreateProgram();
    for (GLuint shader : shaders)
    {
        glAttachShader(program, shader);
    }
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        errorOut.resize(length);
        glGetProgramInfoLog(program, length, nullptr, errorOut.data());
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

const char *Shader::ShaderTypeName(GLenum type)
{
    switch (type)
    {
    case GL_VERTEX_SHADER:
        return "Vertex";
    case GL_FRAGMENT_SHADER:
        return "Fragment";
    case GL_GEOMETRY_SHADER:
        return "Geometry";
    case GL_TESS_CONTROL_SHADER:
        return "Tessellation Control";
    case GL_TESS_EVALUATION_SHADER:
        return "Tessellation Evaluation";
    case GL_COMPUTE_SHADER:
        return "Compute";
    default:
        return "Unknown";
    }
}
