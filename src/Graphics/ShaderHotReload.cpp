#include "Graphics/ShaderHotReload.h"
#include "Graphics/Shader.h"
#include "Graphics/ComputeShader.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include "Util/GLog.h"

void ShaderHotReload::Watch(Shader *shader, ReloadCallback callback)
{
    if (!shader)
        return;
    WatchedEntry entry;
    entry.shader = shader;
    entry.callback = callback;
    for (auto &[type, path] : shader->GetShaderPaths())
    {
        entry.fileTimestamps[path] = GetFileTime(path);
    }

    m_WatchedEntries.push_back(entry);
    LOG_INFO("[HotReload] watching shaders");
    // LOG_INFO("[HotReload] Watching: ", shader->GetVertexPath());
    // LOG_INFO("[HotReload] Watching: ", shader->GetFragmentPath());
}

void ShaderHotReload::Watch(ComputeShader *shader, ReloadCallback callback)
{
    if (!shader)
        return;

    WatchedEntry entry;
    entry.computeShader = shader;
    entry.callback = callback;
    entry.fileTimestamps[shader->GetComputePath()] = GetFileTime(shader->GetComputePath());

    m_WatchedEntries.push_back(std::move(entry));

    std::cout << "[HotReload] Watching Compute Shaders: " << shader->GetComputePath() << std::endl;
}

void ShaderHotReload::Unwatch(Shader *shader)
{
    // todo 改成erase_if
    auto it = std::remove_if(m_WatchedEntries.begin(), m_WatchedEntries.end(),
                             [shader](const WatchedEntry &e)
                             { return e.shader == shader; });
    m_WatchedEntries.erase(it, m_WatchedEntries.end());
}

void ShaderHotReload::Unwatch(ComputeShader *shader)
{
    auto it = std::remove_if(m_WatchedEntries.begin(), m_WatchedEntries.end(),
                             [shader](const WatchedEntry &e)
                             { return e.computeShader == shader; });
    m_WatchedEntries.erase(it, m_WatchedEntries.end());
}

void ShaderHotReload::Update()
{
    static auto lastCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastCheck).count();
    if (elapsed < m_CheckInterval)
    {
        return;
    }
    lastCheck = now;
    for (auto &entry : m_WatchedEntries)
    {
        if (HasFileChanged(entry))
        {
            LOG_INFO("[HotReload] file changed, reloading");
            bool success = false;
            std::string error;
            if (entry.shader)
            {
                success = entry.shader->Reload();
                error = entry.shader->GetLastError();
            }
            else if (entry.computeShader)
            {
                success = entry.computeShader->Reload();
                error = entry.computeShader->GetLastError();
            }
            if (entry.callback)
            {
                entry.callback(success, error);
            }
            if (success)
            {
                LOG_INFO("[HotReload] hot reload success");
            }
            else
            {
                LOG_INFO("[HotReload] hot reload fail: ", error);
            }

            UpdateTimestamps(entry);
        }
    }
}

std::filesystem::file_time_type ShaderHotReload::GetFileTime(const std::string &path)
{
    try
    {
        if (std::filesystem::exists(path))
        {
            return std::filesystem::last_write_time(path);
        }
    }
    catch (...)
    {
    }
    return std::filesystem::file_time_type{};
}

bool ShaderHotReload::HasFileChanged(WatchedEntry &entry)
{
    for (auto &[path, lastTime] : entry.fileTimestamps)
    {
        if (GetFileTime(path) != lastTime)
        {
            return true;
        }
    }
    return false;
}

void ShaderHotReload::UpdateTimestamps(WatchedEntry &entry)
{
    for (auto &[path, lastTime] : entry.fileTimestamps)
    {
        lastTime = GetFileTime(path);
    }
}
