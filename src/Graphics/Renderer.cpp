#include "Graphics/Renderer.h"
#include "Util/GLog.h"

// docs.gl
void TestQuerys()
{
    GLint maxSamples;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples);
    LOG_INFO("最大颜色纹理采样数: ", maxSamples);

    GLint maxTexSize, maxVertexAttribs, maxTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);             // 最大纹理尺寸
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);     // 最大顶点属性数
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits); // 最大纹理单元数

    LOG_INFO("最大纹理尺寸: ", maxTexSize, " x ", maxTexSize);
    LOG_INFO("最大顶点属性: ", maxVertexAttribs);
    LOG_INFO("最大纹理单元: ", maxTextureUnits);

    const char *vendor = (const char *)glGetString(GL_VENDOR);     // GPU 厂商
    const char *renderer = (const char *)glGetString(GL_RENDERER); // GPU 型号
    const char *version = (const char *)glGetString(GL_VERSION);   // OpenGL 版本

    LOG_INFO("GPU: ", vendor, " ", renderer);
    LOG_INFO("OpenGL: ", version);
}

void Renderer::Init()
{
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    {
#ifndef NDEBUG
        TestQuerys();
#endif
    }

    glGenQueries(1, &m_TimerQuery);
}

void Renderer::Clear(float r, float g, float b, float a)
{
    // 清屏颜色设置
    glClearColor(r, g, b, a);
    // clear 颜色缓冲 和 深度缓冲(重置为最远值)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void Renderer::BeginGPUTimer()
{
    glBeginQuery(GL_TIME_ELAPSED, m_TimerQuery);
}

void Renderer::EndGPUTimer()
{
    glEndQuery(GL_TIME_ELAPSED);

    GLuint64 elapsed = 0;
    glGetQueryObjectui64v(m_TimerQuery, GL_QUERY_RESULT, &elapsed);
    m_GPUTimeMs = static_cast<float>(elapsed) / 1000000.0f; // ns to ms
}
