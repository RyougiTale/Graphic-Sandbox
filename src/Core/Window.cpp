#include "Core/Window.h"
#include "Util/GLog.h"
#include <stdexcept>
#include <iostream>
#include <cmath>

Window::Window(int width, int height, const std::string &title)
    : m_Width(width), m_Height(height)
{
    if (!glfwInit())
    {
        LOG_ERROR("GLFW initialize fail");
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // core 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    {
        const char *monitor_name = glfwGetMonitorName(monitor);
        LOG_DEBUG("primary 显示器: ", monitor_name);
        int _x, _y;
        glfwGetMonitorPos(monitor, &_x, &_y);
        LOG_DEBUG("glfwGetMonitorPos: ", _x, " ", _y);
        glfwGetMonitorPhysicalSize(monitor, &_x, &_y);
        LOG_DEBUG("glfwGetMonitorPhysicalSize: ", _x, " ", _y, " 单位mm");
        float _fx, _fy;
        glfwGetMonitorContentScale(monitor, &_fx, &_fy);
        LOG_DEBUG("glfwGetMonitorContentScale: ", _fx, " ", _fy);
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        LOG_DEBUG("width: ", mode->width, " height: ", mode->height, " refresh Rate:", mode->refreshRate);
        LOG_DEBUG("red: ", mode->redBits, " green: ", mode->greenBits, " blue:", mode->blueBits);
        int _w, _h;
        glfwGetMonitorWorkarea(monitor, &_x, &_y, &_w, &_h);
        LOG_DEBUG("glfwGetMonitorWorkarea x: ", _x, " y: ", _y);
        LOG_DEBUG("glfwGetMonitorWorkarea width: ", _w, " height: ", _h);
        const GLFWgammaramp *ramp = glfwGetGammaRamp(monitor);
        LOG_DEBUG("Gamma ramp size: ", ramp->size);
        LOG_DEBUG("red[0]: ", ramp->red[0], "  red[128]: ", ramp->red[128], "  red[255]: ", ramp->red[255]);
        // 中间点反推: output = input^(1/gamma)
        // red[128]/65535 = (128/255)^(1/gamma)
        float output = ramp->red[128] / 65535.0f;
        float input = 128.0f / 255.0f;
        float gamma = 1.0f / (log(output) / log(input));
        LOG_DEBUG("估算 gamma: ", gamma);
    }

    // dpi
    float scaleX, scaleY;
    glfwGetMonitorContentScale(monitor, &scaleX, &scaleY);
    m_ContentScaleX = scaleX;
    m_ContentScaleY = scaleY;

    // workarea
    int workX, workY, workW, workH;
    glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);

    // 默认
    if (width <= 0 || height <= 0)
    {
        m_Width = static_cast<int>(workW * 0.80f);
        m_Height = static_cast<int>(workH * 0.80f);
    }

    m_Window = glfwCreateWindow(m_Width, m_Height, title.c_str(), nullptr, nullptr);
    if (!m_Window)
    {
        glfwTerminate();
        LOG_ERROR("create window fail");
        throw std::runtime_error("Failed to create GLFW window");
    }

    // 居中
    int posX = workX + (workW - m_Width) / 2;
    int posY = workY + (workH - m_Height) / 2;
    glfwSetWindowPos(m_Window, posX, posY);

    glfwMakeContextCurrent(m_Window);
    // 居然是这样传this的...
    glfwSetWindowUserPointer(m_Window, this);
    // 窗口size回调
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
    // 滚轮回调
    glfwSetScrollCallback(m_Window, ScrollCallback);

    // glad动态链接(opengl实现在GPU)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_ERROR("load opengl(glad) fail");
        throw std::runtime_error("Failed to initialize glad");
    }

    LOG_INFO("OpenGL ", glGetString(GL_VERSION));
    LOG_INFO("Renderer: ", glGetString(GL_RENDERER));
    LOG_INFO("Work area: ", workW, " x ", workH);
    LOG_INFO("Cur AspectRatio: ", GetAspectRatio());
    LOG_INFO("Window: ", m_Width, " x ", m_Height, " (DPI scale: ", m_ContentScaleX, ")");

    // VSync 垂直同步
    // 0 关闭VSync, 不等待显示器刷新, 可能撕裂, todo: 模拟撕裂
    // 1 每帧等待显示器刷新(锁帧)
    // 2 显示器减少一半的刷新频率
    glfwSwapInterval(0);
}

Window::~Window()
{
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}

// close事件检测
bool Window::ShouldClose() const
{
    // LOG_DEBUG("ShouldClose");
    return glfwWindowShouldClose(m_Window);
}

// events poll
void Window::PollEvents()
{
    // LOG_DEBUG("PollEvents");
    glfwPollEvents();
}

// 交换前后缓冲区
void Window::SwapBuffers()
{
    // LOG_DEBUG("SwapBuffers");
    glfwSwapBuffers(m_Window);
}

bool Window::IsKeyPressed(int key) const
{
    // LOG_DEBUG("IsKeyPressed");
    return glfwGetKey(m_Window, key) == GLFW_PRESS;
}

bool Window::IsMouseButtonPressed(int button) const
{
    // LOG_DEBUG("IsMouseButtonPressed");
    return glfwGetMouseButton(m_Window, button) == GLFW_PRESS;
}

void Window::GetCursorPos(double &x, double &y) const
{
    LOG_DEBUG("GetCursorPos");
    glfwGetCursorPos(m_Window, &x, &y);
}

// cursor模式 (显示 / 隐藏 / 锁定)
void Window::SetCursorMode(int mode)
{
    LOG_DEBUG("SetCursorMode");
    glfwSetInputMode(m_Window, GLFW_CURSOR, mode);
}

// 多回调
// std::vector<std::function<void(int, int)>> OnResizeCallbacks;
// void AddResizeCallback(std::function<void(int, int)> callback) {
//     OnResizeCallbacks.push_back(std::move(callback));
// }
// void FramebufferSizeCallback(GLFWwindow* window, int w, int h) {
//     for (auto& cb : win->OnResizeCallbacks) {
//         cb(w, h);
//     }
// }

void Window::FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    Window *win = static_cast<Window *>(glfwGetWindowUserPointer(window));
    win->m_Width = width;
    win->m_Height = height;
    glViewport(0, 0, width, height);
    if (win->OnResize)
    {
        win->OnResize(width, height);
    }
}

void Window::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    Window *win = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (win->OnScroll)
    {
        win->OnScroll(xoffset, yoffset);
    }
}
