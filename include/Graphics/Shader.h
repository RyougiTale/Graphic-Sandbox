#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&other) noexcept;
    Shader &operator=(Shader &&other) noexcept;

    bool LoadFromFiles(const std::string &vertexPath, const std::string &fragmentPath);
    bool Reload();

    void Use() const;
    GLuint GetProgram() const { return m_Program; }
    bool IsValid() const { return m_Program != 0; }

    const std::string &GetLastError() const { return m_LastError; }
    const std::string &GetVertexPath() const { return m_VertexPath; }
    const std::string &GetFragmentPath() const { return m_FragmentPath; }

    // Uniform setters
    void SetInt(const std::string &name, int value);
    void SetFloat(const std::string &name, float value);
    void SetVec2(const std::string &name, const glm::vec2 &value);
    void SetVec3(const std::string &name, const glm::vec3 &value);
    void SetVec4(const std::string &name, const glm::vec4 &value);
    void SetMat3(const std::string &name, const glm::mat3 &value);
    void SetMat4(const std::string &name, const glm::mat4 &value);

private:
    GLint GetUniformLocation(const std::string &name);
    static std::string ReadFile(const std::string &path);
    static GLuint CompileShader(GLenum type, const std::string &source, std::string &errorOut);
    static GLuint LinkProgram(GLuint vertex, GLuint fragment, std::string &errorOut);

    GLuint m_Program = 0;
    std::string m_VertexPath;
    std::string m_FragmentPath;
    std::string m_LastError;
    std::unordered_map<std::string, GLint> m_UniformCache;
};
