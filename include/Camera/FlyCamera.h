#pragma once

#include "ICamera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Window;
// 第一人称camera
class FlyCamera : public ICamera
{
public:
    FlyCamera();

    void Update(float deltaTime, Window &window);
    void OnScroll(double yoffset);
    glm::mat4 GetViewMatrix() const override;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const override;
    void RenderImGui() override;

    // 位置
    glm::vec3 position{0.0f, 0.0f, 5.0f};
    // 飞行器术语
    // Pitch: 俯仰, 绕X轴, 值从-89到89, 0表示平视
    // Yaw: 偏航, 绕Y轴, 值从-180到180， 0表示看向X(标准三角函数单位元), -90看向-Z
    // Roll: 翻滚(歪头, 一般不用, 会被m_WorldUp锁定)
    float yaw = -90.0f;
    float pitch = 0.0f;
    // 速度
    float moveSpeed = 15.0f;
    float sprintMultiplier = 2.0f;
    float mouseSensitivity = 0.15f;
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

private:
    void UpdateVectors();

    // 右手坐标系, 相机看向的方向
    glm::vec3 m_Front{0.0f, 0.0f, -1.0f};
    // 相机头顶的方向
    glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
    // 相机右边的方向
    glm::vec3 m_Right{1.0f, 0.0f, 0.0f};
    // 世界的上方向(固定不变), 用来旋转时计算m_Up和m_Right
    glm::vec3 m_WorldUp{0.0f, 1.0f, 0.0f};

    double m_LastMouseX = 0.0;
    double m_LastMouseY = 0.0;
    bool m_FirstMouse = true;
    bool m_WasRightMousePressed = false;
};
