#include "Util/Timer.h"

Timer::Timer() : m_StartTime(Clock::now()), m_LastFrameTime(Clock::now()) {}

void Timer::Update()
{
    auto current_time = Clock::now();
    std::chrono::duration<float> delta = current_time - m_LastFrameTime;
    m_DeltaTime = delta.count();
    std::chrono::duration<float> total = current_time - m_StartTime;
    m_TotalTime = total.count();
    m_LastFrameTime = current_time;

    // FPS
    m_FrameCount++;
    m_FPSTimer += m_DeltaTime;
    // FPS计算区间(灵敏度)
    if (m_FPSTimer >= 0.5f)
    {
        m_FPS = static_cast<float>(m_FrameCount) / m_FPSTimer;
        m_FrameCount = 0;
        m_FPSTimer = 0.0f;
    }
}