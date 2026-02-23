#pragma once

#include <vector>
#include <string>

class PerformancePanel
{
public:
    void Update(float deltaTime, float gpuTimeMs, int windowWidth, int windowHeight);
    void Render();

    std::string shaderError;
    std::string shaderStatus = "OK";

    bool overrideAspectRatio = false;
    float customAspectRatio = 1.78f;

private:
    static constexpr int HISTORY_SIZE = 120;

    std::vector<float> m_FrameTimeHistory;
    std::vector<float> m_GPUTimeHistory;
    float m_FPS = 0.0f;
    float m_FrameTimeMs = 0.0f;
    float m_GPUTimeMs = 0.0f;

    float m_AvgFrameTime = 0.0f;
    float m_AvgGPUTime = 0.0f;

    int m_WindowWidth = 0;
    int m_WindowHeight = 0;
};