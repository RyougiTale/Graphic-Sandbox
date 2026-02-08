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

    m_Window.OnResize = [this](int w, int h)
    {
        m_Renderer.SetViewport(0, 0, w, h);
        // m_DemoManager.OnResize(w, h);
    };
}

void Application::MainLoop()
{
    m_Timer.Update();
    m_Window.PollEvents();

    float delta = m_Timer.GetDeltaTime();

    // ESC
    if (m_Window.IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        m_Running = false;
    }

    m_Renderer.Clear(0.1f, 0.1f, 0.12f);

    m_Renderer.BeginGPUTimer();

    m_Renderer.EndGPUTimer();
    m_Window.SwapBuffers();
}

void Application::ShutDown()
{
}

void Application::Run()
{
    LOG_DEBUG("Run");
    while (!m_Window.ShouldClose() && m_Running)
    {
        MainLoop();
    }
}
