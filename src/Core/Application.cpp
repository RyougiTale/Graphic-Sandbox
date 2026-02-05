#include <iostream>
#include "Core/Application.h"
#include <GLFW/glfw3.h>

Application::Application(int width, int height, const char *title) : m_window(width, height, title)
{
    Init();
}

Application::~Application()
{
    ShutDown();
}

void Application::Init()
{
}

void Application::MainLoop()
{
}

void Application::ShutDown()
{
}

void Application::Run()
{
    std::cout << "hello" << std::endl;
}
