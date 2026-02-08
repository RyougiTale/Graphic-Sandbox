#include "IMGUI/ImGuiLayer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

void ImGuiLayer::Init(GLFWwindow* window, float contentScale) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    // 键盘导航, tab 方向键切换focus
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // 启用窗口停靠
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // 允许变成独立窗口
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // 手柄
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    // 缩放
    // io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaling;

    // font size based on DPI
    float fontSize = 18.0f * contentScale;
    ImFontConfig fontConfig;
    fontConfig.SizePixels = fontSize;
    // 2倍分辨率渲染再缩回
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    io.Fonts->AddFontDefault(&fontConfig);
    // todo 矢量字体
    // io.Fonts->AddFontFromFileTTF("fonts/NotoSansSC-Regular.ttf", 18.0f * contentScale);


    // ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    ImGui::StyleColorsClassic();

    // style with DPI scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f; // 窗口圆角半径
    style.FrameRounding = 2.0f; // 按钮 输入框圆角半径
    style.ScrollbarRounding = 2.0f; // 滚动条圆角
    style.ScaleAllSizes(contentScale);

    // 后端初始化绑定glfw
    // 注册键盘, 鼠标回调
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // imgui内部shader版本声明
    // 后端初始化绑定到opengl
    // 创建imgui渲染用的shader buffer等
    ImGui_ImplOpenGL3_Init("#version 460");
}

void ImGuiLayer::Shutdown()
{
    // opengl3(依托于GL上下文)先释放
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame(); // 渲染后端状态
    ImGui_ImplGlfw_NewFrame(); // 读取输入, 窗口状态
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiLayer::WantCaptureMouse() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::WantCaptureKeyboard() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}
