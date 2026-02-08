#pragma once

struct GLFWwindow;

class ImGuiLayer
{
public:
    void Init(GLFWwindow *window, float contentScale = 1.0f);
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;
};