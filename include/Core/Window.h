// Window封装
#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

/*
while (!window.ShouldClose()) {
    window.PollEvents();
    if (window.IsKeyPressed(GLFW_KEY_ESCAPE))
        break;
    Update();
    Render();
    window.SwapBuffers();
}
*/
class Window
{
public:
    Window(int width, int height, const std::string &title);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    // 封装GLFW
    bool ShouldClose() const;
    void PollEvents();
    void SwapBuffers();

    GLFWwindow *GetHandle() const { return m_Window; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    float GetAspectRatio() const { return static_cast<float>(m_Width) / static_cast<float>(m_Height); }
    float GetContentScale() const { return m_ContentScaleX; }

    bool IsKeyPressed(int key) const;
    bool IsMouseButtonPressed(int button) const;
    void GetCursorPos(double &x, double &y) const;
    void SetCursorMode(int mode);

    std::function<void(int, int)> OnResize;
    std::function<void(double, double)> OnScroll;

private:
    static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    // window pointer
    GLFWwindow *m_Window = nullptr;
    // window 属性
    int m_Width;
    int m_Height;
    float m_ContentScaleX = 1.0f;
    float m_ContentScaleY = 1.0f;
};
