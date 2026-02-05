#pragma once
#include <Core/Window.h>

class Application
{
public:
    Application(int width, int height, const char *title);
    ~Application();

    void Run();

private:
    void Init();
    void MainLoop();
    void ShutDown();

    Window m_window;
};
