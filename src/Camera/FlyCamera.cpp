#include "Camera/FlyCamera.h"
#include "Core/Window.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <imgui.h>

FlyCamera::FlyCamera()
{
    UpdateVectors();
}

// 每帧都调用
// 为什么不在glfwSetKeyCallback监听
// 因为时序不同步
void FlyCamera::Update(float deltaTime, Window &window)
{
    bool rightMousePressed = window.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

    // 检测刚刚按下
    if (rightMousePressed && !m_WasRightMousePressed)
    {
        window.SetCursorMode(GLFW_CURSOR_DISABLED);
        m_FirstMouse = true;
    }
    // 检测刚刚松开
    else if (!rightMousePressed && m_WasRightMousePressed)
    {
        window.SetCursorMode(GLFW_CURSOR_NORMAL);
    }
    m_WasRightMousePressed = rightMousePressed;

    // 锁cursor
    // 按下右键才计算旋转
    if (rightMousePressed)
    {
        double mouseX, mouseY;
        window.GetCursorPos(mouseX, mouseY);

        if (m_FirstMouse)
        {
            m_LastMouseX = mouseX;
            m_LastMouseY = mouseY;
            m_FirstMouse = false;
        }

        float xoffset = static_cast<float>(mouseX - m_LastMouseX) * mouseSensitivity;
        // windows窗口y轴向下
        float yoffset = static_cast<float>(m_LastMouseY - mouseY) * mouseSensitivity;

        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;

        yaw += xoffset;
        pitch += yoffset;

        // 避免万向节死锁
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        UpdateVectors();
    }

    // move
    float speed = moveSpeed * deltaTime;
    if (window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
    {
        speed *= sprintMultiplier;
    }

    if (window.IsKeyPressed(GLFW_KEY_W))
        position += m_Front * speed;
    if (window.IsKeyPressed(GLFW_KEY_S))
        position -= m_Front * speed;
    if (window.IsKeyPressed(GLFW_KEY_A))
        position -= m_Right * speed;
    if (window.IsKeyPressed(GLFW_KEY_D))
        position += m_Right * speed;
    if (window.IsKeyPressed(GLFW_KEY_E))
        position += m_WorldUp * speed;
    if (window.IsKeyPressed(GLFW_KEY_Q))
        position -= m_WorldUp * speed;
}

// yoffset glfw的滚轮回调
// 速度增加或减少0.5/per
void FlyCamera::OnScroll(double yoffset)
{
    moveSpeed += static_cast<float>(yoffset) * 0.5f;
    // clamp: 限制在一个范围
    moveSpeed = std::clamp(moveSpeed, 0.5f, 50.0f);
}

// 用于改变世界(World Space 转化到 View Space)
// 输出: 返回一个4X4逆变换矩阵View Matrix
glm::mat4 FlyCamera::GetViewMatrix() const
{
    // glm::lookAt(eye, center, up)
    // 摄像机位置, 看向的点(目标方向上任意点), 头顶朝向
    return glm::lookAt(position, position + m_Front, m_Up);
}

// 生成透视投影矩阵Perspective Projection Matrix
// 透视除法(近大远小): x和y都除以z
// MVP的最后一步
glm::mat4 FlyCamera::GetProjectionMatrix(float aspectRatio) const
{
    // glm::perspective
    // fov: 视野角度(Field of View), aspectRatio: 宽高比(修正屏幕比例)
    // nearPlane: 近裁剪面, 能看到的最近距离, 更近的会被clip, 不进行渲染
    // farPlain: 远裁剪面, 最远距离, 超过会被clip, 太大会导致Z-fighting
    // 深度值z buffer和实际距离z view大致是反比关系
    // 近处分了90%精度, 远处分了10%精度
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void FlyCamera::RenderImGui()
{
    ImGui::SetNextWindowPos(ImVec2(20, 375), ImGuiCond_FirstUseEver);
    ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat3("Position", &position.x, 0.1f);
    bool changed = ImGui::SliderFloat("Yaw", &yaw, -180.0f, 180.0f);
    changed |= ImGui::SliderFloat("Pitch", &pitch, -89.0f, 89.0f);
    if (changed)
    {
        UpdateVectors();
    }
    ImGui::Separator();
    ImGui::SliderFloat("Move Speed", &moveSpeed, 0.5f, 50.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 1.0f);
    ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f);
    ImGui::Separator();
    if (ImGui::Button("Reset"))
    {
        position = glm::vec3(0.0f, 2.0f, 5.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        moveSpeed = 15.0f;
        mouseSensitivity = 0.15f;
        fov = 60.0f;
        UpdateVectors();
    }
    ImGui::End();
}

// glm::vec3 m_Front{0.0f, 0.0f, -1.0f};
// glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
// glm::vec3 m_Right{1.0f, 0.0f, 0.0f};
// glm::vec3 m_WorldUp{0.0f, 1.0f, 0.0f};
void FlyCamera::UpdateVectors()
{
    // Radians 角度->弧度
    // cos(radians(pitch))是xz的投影长度
    // 之后根据单位元方向计算可得front

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    m_Front = glm::normalize(front);
    // front是视线方向, 叉乘朝右
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    // 叉乘(右手定则)
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}