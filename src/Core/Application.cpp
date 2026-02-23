#include <iostream>
#include "Core/Application.h"
#include <GLFW/glfw3.h>
#include "Util/GLog.h"
#include "Demos/TriangleDemo.h"
#include "Demos/ComputeDemo.h"

Application::Application(int width, int height, const char *title) : m_Window(width, height, title)
{
    Init();
}

Application::~Application()
{
    ShutDown();
}

void Application::Init()
{
    m_Renderer.Init();
    m_ImGui.Init(m_Window.GetHandle(), m_Window.GetContentScale());

    m_Window.OnResize = [this](int w, int h)
    {
        m_Renderer.SetViewport(0, 0, w, h);
        m_DemoManager.OnResize(w, h);
    };

    m_Window.OnScroll = [this](double, double y)
    {
        if (!m_ImGui.WantCaptureMouse())
        {
            m_Camera.OnScroll(y);
        }
    };
    LOG_INFO("register demos begin");
    // 注册demos
    m_DemoManager.Register<TriangleDemo>();
    m_DemoManager.Register<ComputeDemo>();
    // 注册结束
    LOG_INFO("register demos end");
    m_DemoManager.Init(m_ShaderHotReload);
    LOG_INFO("demo manager init end");
}

void Application::MainLoop()
{
    // LOG_DEBUG("MainLoop");
    // 时间 帧率相关计算
    m_Timer.Update();
    // 事件poll
    m_Window.PollEvents();

    // ESC
    if (m_Window.IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        m_Running = false;
    }
    // deltaTime CPU测的耗时
    float delta = m_Timer.GetDeltaTime();
    // imgui的focus判断
    if (!m_ImGui.WantCaptureMouse() && !m_ImGui.WantCaptureKeyboard())
    {
        m_Camera.Update(delta, m_Window);
    }
    // shader hot reload检测
    m_ShaderHotReload.Update();
    // 自顶向下, demo的update
    m_DemoManager.Update(delta);

    // render流程
    m_Renderer.Clear(0.1f, 0.1f, 0.12f);
    // render干活
    // gpuTimeMs GPU实际干活时间
    m_Renderer.BeginGPUTimer();
    m_DemoManager.Render(m_Camera);
    m_Renderer.EndGPUTimer();

    // imgui 部分
    m_ImGui.BeginFrame();
    m_PerfPanel.Update(delta, m_Renderer.GetGPUTimeMs());
    m_PerfPanel.Render();
    m_Camera.RenderImGui();
    m_DemoManager.RenderImGui();

    m_ImGui.EndFrame();
    // 全屏模式可以绕过Desktop Window Manager直接输出到显示器
    // 交换指针
    m_Window.SwapBuffers();
}

void Application::ShutDown()
{
    m_DemoManager.Shutdown();
    m_ImGui.Shutdown();
}

void Application::Run()
{
    LOG_DEBUG("Run");
    while (!m_Window.ShouldClose() && m_Running)
    {
        MainLoop();
    }
}
