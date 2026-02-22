#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class ComputeShader
{
public:
    ComputeShader() = default;
    ~ComputeShader();

    ComputeShader(const ComputeShader &) = delete;
    ComputeShader &operator=(const ComputeShader &) = delete;
    ComputeShader(ComputeShader &&other) noexcept;
    ComputeShader &operator=(ComputeShader &&other) noexcept;

    bool LoadFromFile(const std::string &computePath);
    bool Reload();

    void Use() const;
    void Dispatch(GLuint numGroupsX, GLuint numGroupsY = 1, GLuint numGroupsZ = 1) const;
    GLuint GetProgram() const { return m_Program; }
    bool IsValid() const { return m_Program != 0; }

    const std::string &GetComputePath() const { return m_ComputePath; }
    const std::string &GetLastError() const { return m_LastError; }

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

    GLuint m_Program = 0;
    std::string m_ComputePath;
    std::string m_LastError;
    std::unordered_map<std::string, GLint> m_UniformCache;
};
