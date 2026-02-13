#include <iostream>
#include "Core/Application.h"
#include <GLFW/glfw3.h>
#include "Util/GLog.h"

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
        // m_DemoManager.OnResize(w, h);
    };

    m_Window.OnScroll = [this](double x, double y)
    {
        if (!m_ImGui.WantCaptureMouse())
        {
            m_Camera.OnScroll(y);
        }
    };
}

void Application::MainLoop()
{
    LOG_DEBUG("MainLoop");
    m_Timer.Update();
    m_Window.PollEvents();

    // ESC
    if (m_Window.IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        m_Running = false;
    }
    float delta = m_Timer.GetDeltaTime();
    // deltaTime CPU测的耗时
    if (!m_ImGui.WantCaptureMouse() && !m_ImGui.WantCaptureKeyboard())
    {
        m_Camera.Update(delta, m_Window);
    }

    m_Renderer.Clear(0.1f, 0.1f, 0.12f);

    m_Renderer.BeginGPUTimer();
    // gpuTimeMs GPU实际干活时间

    m_Renderer.EndGPUTimer();

    // imgui
    m_ImGui.BeginFrame();

    m_PerfPanel.Update(delta, m_Renderer.GetGPUTimeMs());
    m_PerfPanel.Render();
    // m_DemoManager.RenderImGui();

    m_ImGui.EndFrame();
    // swap buffer
    m_Window.SwapBuffers();
}

void Application::ShutDown()
{
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
