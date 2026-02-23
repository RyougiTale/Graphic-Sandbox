#include "ImGui/PerformancePanel.h"
#include "Camera/FlyCamera.h"
#include <imgui.h>
#include <numeric>
#include <algorithm>

void PerformancePanel::Update(float deltaTime, float gpuTimeMs, int windowWidth, int windowHeight)
{
    // 秒转到毫秒
    m_FrameTimeMs = deltaTime * 1000.0f;
    m_GPUTimeMs = gpuTimeMs;
    m_FPS = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
    m_WindowWidth = windowWidth;
    m_WindowHeight = windowHeight;

    m_FrameTimeHistory.push_back(m_FrameTimeMs);
    m_GPUTimeHistory.push_back(m_GPUTimeMs);

    if (m_FrameTimeHistory.size() > HISTORY_SIZE)
    {
        m_FrameTimeHistory.erase(m_FrameTimeHistory.begin());
    }
    if (m_GPUTimeHistory.size() > HISTORY_SIZE)
    {
        m_GPUTimeHistory.erase(m_GPUTimeHistory.begin());
    }

    if (!m_FrameTimeHistory.empty())
    {
        m_AvgFrameTime = std::accumulate(m_FrameTimeHistory.begin(), m_FrameTimeHistory.end(), 0.0f);
        m_AvgFrameTime /= static_cast<float>(m_FrameTimeHistory.size());
    }
    if (!m_GPUTimeHistory.empty())
    {
        m_AvgGPUTime = std::accumulate(m_GPUTimeHistory.begin(), m_GPUTimeHistory.end(), 0.0f);
        m_AvgGPUTime /= static_cast<float>(m_GPUTimeHistory.size());
    }
}

void PerformancePanel::Render()
{
    // 初始化窗口
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // FPS and frame time
    ImGui::Text("FPS: %.1f", m_FPS);
    ImGui::Text("Frame Time: %.2f ms (avg: %.2f ms)", m_FrameTimeMs, m_AvgFrameTime);
    ImGui::Text("GPU Time: %.2f ms (avg: %.2f ms)", m_GPUTimeMs, m_AvgGPUTime);
    ImGui::Text("Viewport: %dx%d (%.2f)", m_WindowWidth, m_WindowHeight,
                m_WindowHeight > 0 ? static_cast<float>(m_WindowWidth) / static_cast<float>(m_WindowHeight) : 0.0f);
    ImGui::Checkbox("Override Aspect Ratio", &overrideAspectRatio);
    if (overrideAspectRatio)
    {
        ImGui::SliderFloat("Aspect Ratio", &customAspectRatio, 0.2f, 5.0f);
    }

    ImGui::Separator();

    // Frame time graph
    if (!m_FrameTimeHistory.empty())
    {
        float maxTime = *std::max_element(m_FrameTimeHistory.begin(), m_FrameTimeHistory.end());
        maxTime = std::max(maxTime, 16.67f);

        ImGui::PlotLines("Frame Time (ms)", m_FrameTimeHistory.data(),
                         static_cast<int>(m_FrameTimeHistory.size()), 0, nullptr, 0.0f, maxTime,
                         ImVec2(0, 60));
    }

    // GPU time graph
    if (!m_GPUTimeHistory.empty())
    {
        float maxTime = *std::max_element(m_GPUTimeHistory.begin(), m_GPUTimeHistory.end());
        maxTime = std::max(maxTime, 16.67f);

        ImGui::PlotLines("GPU Time (ms)", m_GPUTimeHistory.data(),
                         static_cast<int>(m_GPUTimeHistory.size()), 0, nullptr, 0.0f, maxTime,
                         ImVec2(0, 60));
    }

    ImGui::Separator();

    // Shader status
    if (!shaderError.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("Shader Error:");
        ImGui::TextWrapped("%s", shaderError.c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
        ImGui::Text("Shader: %s", shaderStatus.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::End();
}
