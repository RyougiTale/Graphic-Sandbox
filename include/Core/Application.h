#pragma once
#include <Core/Window.h>
#include "Util/Timer.h"
#include "Graphics/Renderer.h"
#include "ImGui/ImGuiLayer.h"
#include "ImGui/PerformancePanel.h"
#include "Camera/FlyCamera.h"
#include "Demo/DemoManager.h"
#include "Graphics/ShaderHotReload.h"
#include "Graphics/DebugDraw.h"

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
    PerformancePanel m_PerfPanel;
    FlyCamera m_Camera;
    DemoManager m_DemoManager;
    ShaderHotReload m_ShaderHotReload;
    DebugDraw m_DebugDraw;

    bool m_Running = true;
};
