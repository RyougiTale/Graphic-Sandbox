#pragma once

#include <glad/glad.h>

class Renderer
{
public:
    void Init();
    void Clear(float r = 0.1f, float g = 0.1f, float b = 0.1f, float a = 1.0f);
    void SetViewport(int x, int y, int width, int height);

    // GPU timing
    void BeginGPUTimer();
    void EndGPUTimer();
    float GetGPUTimeMs() const { return m_GPUTimeMs; }

private:
    GLuint m_TimerQuery = 0;
    float m_GPUTimeMs = 0.0f;
};
