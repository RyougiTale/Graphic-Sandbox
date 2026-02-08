#pragma once
#include <Core/Window.h>
#include "Util/Timer.h"
#include "Graphics/Renderer.h"
#include "ImGui/ImGuiLayer.h"

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

    
    Window m_Window;
    Timer m_Timer;
    Renderer m_Renderer;
    ImGuiLayer m_ImGui;

    bool m_Running = true;
};
