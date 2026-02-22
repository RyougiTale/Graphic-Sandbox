#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <map>
#include <glad/glad.h>

class Shader;
class ComputeShader;

class ShaderHotReload
{
public:
    using ReloadCallback = std::function<void(bool success, const std::string &error)>;

    void Watch(Shader *shader, ReloadCallback callback = nullptr);
    void Watch(ComputeShader *shader, ReloadCallback callback = nullptr);
    void Unwatch(Shader *shader);
    void Unwatch(ComputeShader *shader);
    void Update(); // Call every frame

    float GetCheckInterval() const { return m_CheckInterval; }
    void SetCheckInterval(float seconds) { m_CheckInterval = seconds; }

private:
    struct WatchedEntry
    {
        Shader *shader = nullptr;
        ComputeShader *computeShader = nullptr;
        std::map<std::string, std::filesystem::file_time_type> fileTimestamps;
        ReloadCallback callback;
    };

    std::filesystem::file_time_type GetFileTime(const std::string &path);
    bool HasFileChanged(WatchedEntry &entry);
    void UpdateTimestamps(WatchedEntry &entry);

    std::vector<WatchedEntry> m_WatchedEntries;
    float m_CheckInterval = 0.5f; // Check every 0.5 seconds
    float m_TimeSinceLastCheck = 0.0f;
};
