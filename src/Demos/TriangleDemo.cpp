#include "Demos/TriangleDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

void TriangleDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/triangle/triangle.vert",
                           "shaders/Demos/triangle/triangle.frag");

    //         (0, 0.5, 0)
    //              △红
    //             / \
    //            /   \
    //         绿/     \ 蓝
    // (-0.5,-0.5,0)──(0.5,-0.5,0)
    float vertices[] = {
        // Position          // Color
        0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f};

    // Vertex Array Object 顶点数据解读
    glGenVertexArrays(1, &m_VAO);
    // Vertex Buffer Object 一块显存(存顶点数据)
    glGenBuffers(1, &m_VBO);

    // glBindXxx 是 opengl的状态机模式
    // 激活VAO
    glBindVertexArray(m_VAO);
    // 激活VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    // CPU -> GPU 拷贝
    // GL_STATIC_DRAW提示数据不会被频繁修改, 可以放在快速显存里
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    // 告诉GPU怎么读
    // 0对应属性编号, shader里 layout(location=0)
    // 3对应每个属性有几个分量 (xyz)
    // GL_FLOAT是数据类型
    // GL_FALSE是不归一化
    // 6是步长为6
    // 偏移是从0字节开始
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 解绑 VAO 防止后续操作
    glBindVertexArray(0);
}

void TriangleDemo::OnDestroy()
{
    if (m_VAO)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO)
    {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
}

void TriangleDemo::OnUpdate(float deltaTime)
{
    m_Rotation += m_RotationSpeed * deltaTime;
}

void TriangleDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid())
        return;

    m_Shader.Use();

    // 单位矩阵
    glm::mat4 model = glm::mat4(1.0f);
    // 绕y轴旋转m_Rotation弧度(m_Rotation在OnUpdate里累加， 三角形会旋转)
    model = glm::rotate(model, m_Rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    // glm::vec3(m_Scale) 等价于 vec3(m_Scale, m_Scale, m_Scale)
    // 矩阵乘法是从右往左应用的，所以实际变换顺序是：先缩放，再旋转
    model = glm::scale(model, glm::vec3(m_Scale));

    // 内部调用glUniformMatrix4fv / glUniform3f
    m_Shader.SetMat4("u_Model", model);
    m_Shader.SetMat4("u_View", camera.GetViewMatrix());
    m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
    m_Shader.SetVec3("u_TintColor", glm::vec3(m_Color[0], m_Color[1], m_Color[2]));

    // 激活之前配置好的 VAO，GPU 就知道从哪个 VBO 读、按什么布局解析
    glBindVertexArray(m_VAO);
    // 图元类型, 起始索引, 顶点数量(总共读3个顶点)
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void TriangleDemo::OnImGui()
{
    ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 10.0f);
    ImGui::SliderFloat("Scale", &m_Scale, 0.1f, 5.0f);
    ImGui::ColorEdit3("Tint Color", m_Color);

    if (ImGui::Button("Reset"))
    {
        m_Rotation = 0.0f;
        m_RotationSpeed = 1.0f;
        m_Scale = 1.0f;
        m_Color[0] = 1.0f;
        m_Color[1] = 0.5f;
        m_Color[2] = 0.2f;
    }
}

void TriangleDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
