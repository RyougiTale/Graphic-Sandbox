#pragma once

#include <chrono>

class Timer
{
public:
    Timer();

    void Update();
    float GetDeltaTime() const { return m_DeltaTime; }
    float GetTotalTime() const { return m_TotalTime; }
    float GetFPS() const { return m_FPS; }
    float GetFrameTimeMs() const { return m_DeltaTime * 1000.0f; }

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint m_StartTime;
    TimePoint m_LastFrameTime;
    float m_DeltaTime = 0.0f;
    float m_TotalTime = 0.0f;

    // FPS calculation
    float m_FPS = 0.0f;
    int m_FrameCount = 0;
    float m_FPSTimer = 0.0f;
};
