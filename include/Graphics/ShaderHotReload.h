#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

class Shader;

class ShaderHotReload
{
public:
    using ReloadCallback = std::function<void(Shader *, bool success, const std::string &error)>;

    void Watch(Shader *shader, ReloadCallback callback = nullptr);
    void Unwatch(Shader *shader);
    void Update(); // Call every frame

    float GetCheckInterval() const { return m_CheckInterval; }
    void SetCheckInterval(float seconds) { m_CheckInterval = seconds; }

private:
    struct WatchedShader
    {
        Shader *shader;
        std::filesystem::file_time_type vertexLastWrite;
        std::filesystem::file_time_type fragmentLastWrite;
        ReloadCallback callback;
    };

    std::filesystem::file_time_type GetFileTime(const std::string &path);
    bool HasFileChanged(WatchedShader &watched);

    std::vector<WatchedShader> m_WatchedShaders;
    float m_CheckInterval = 0.5f; // Check every 0.5 seconds
    float m_TimeSinceLastCheck = 0.0f;
};
