#include "Demos/TriangleDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

void TriangleDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/triangle/triangle.vert",
                           "shaders/Demos/triangle/triangle.frag");

    // Triangle vertices (position + color)
    float vertices[] = {
        // Position          // Color
        0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f};

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

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

void TriangleDemo::OnRender(const ICamera &camera)
{
    if (!m_Shader.IsValid())
        return;

    m_Shader.Use();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, m_Rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_Scale));

    m_Shader.SetMat4("u_Model", model);
    m_Shader.SetMat4("u_View", camera.GetViewMatrix());
    m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(16.0f / 9.0f)); // TODO: proper aspect
    m_Shader.SetVec3("u_TintColor", glm::vec3(m_Color[0], m_Color[1], m_Color[2]));

    glBindVertexArray(m_VAO);
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
