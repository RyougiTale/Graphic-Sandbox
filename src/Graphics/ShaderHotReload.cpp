#include "Graphics/ShaderHotReload.h"
#include "Graphics/Shader.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include "Util/GLog.h"

void ShaderHotReload::Watch(Shader *shader, ReloadCallback callback)
{
    if (!shader)
        return;
    WatchedShader watched;
    watched.shader = shader;
    watched.vertexLastWrite = GetFileTime(shader->GetVertexPath());
    watched.fragmentLastWrite = GetFileTime(shader->GetFragmentPath());
    watched.callback = callback;

    m_WatchedShaders.push_back(watched);
    LOG_INFO("[HotReload] Watching: ", shader->GetVertexPath());
    LOG_INFO("[HotReload] Watching: ", shader->GetFragmentPath());
}

void ShaderHotReload::Unwatch(Shader *shader)
{
    // 改成erase if
    auto it = std::remove_if(m_WatchedShaders.begin(), m_WatchedShaders.end(),
                             [shader](const WatchedShader &w)
                             { return w.shader == shader; });
    m_WatchedShaders.erase(it, m_WatchedShaders.end());
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
    for (auto &watched : m_WatchedShaders)
    {
        if (HasFileChanged(watched))
        {
            LOG_INFO("[HotReload] file changed, reloading");
            bool success = watched.shader->Reload();
            if (watched.callback)
            {
                watched.callback(watched.shader, success, watched.shader->GetLastError());
            }
            if (success)
            {
                LOG_INFO("[HotReload] hot reload success");
            }
            else
            {
                LOG_INFO("[HotReload] hot reload fail: ", watched.shader->GetLastError());
            }

            watched.vertexLastWrite = GetFileTime(watched.shader->GetVertexPath());
            watched.fragmentLastWrite = GetFileTime(watched.shader->GetFragmentPath());
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

bool ShaderHotReload::HasFileChanged(WatchedShader &watched)
{
    auto vertexTime = GetFileTime(watched.shader->GetVertexPath());
    auto fragmentTime = GetFileTime(watched.shader->GetFragmentPath());

    return vertexTime != watched.vertexLastWrite || fragmentTime != watched.fragmentLastWrite;
}
