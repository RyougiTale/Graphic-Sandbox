#include "Demos/SDFLatticeDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

void SDFLatticeDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/sdf_lattice/sdf_lattice.vert",
                           "shaders/Demos/sdf_lattice/sdf_lattice.frag");
    glGenVertexArrays(1, &m_VAO);
}

void SDFLatticeDemo::OnDestroy()
{
    if (m_VAO)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

void SDFLatticeDemo::OnUpdate(float deltaTime)
{
    if (m_AutoRotate)
        m_RotationAngle += m_RotationSpeed * deltaTime;
}

void SDFLatticeDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid())
        return;

    m_Shader.Use();

    // 相机
    m_Shader.SetMat4("u_InvView", glm::inverse(camera.GetViewMatrix()));
    m_Shader.SetMat4("u_InvProjection", glm::inverse(camera.GetProjectionMatrix(aspectRatio)));
    m_Shader.SetVec3("u_CameraPos", camera.GetPosition());

    // 旋转
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), m_RotationAngle, glm::vec3(0, 1, 0));
    m_Shader.SetMat4("u_Rotation", rotation);
    glm::mat4 invRotation = glm::transpose(rotation); // 正交矩阵的逆 = 转置
    m_Shader.SetMat4("u_InvRotation", invRotation);

    // 晶格参数
    m_Shader.SetInt("u_LatticeType", static_cast<int>(m_Type));
    m_Shader.SetFloat("u_CellSize", m_CellSize);
    m_Shader.SetFloat("u_StrutRadius", m_StrutRadius);
    m_Shader.SetFloat("u_NodeRadius", m_NodeRadius);
    m_Shader.SetFloat("u_SmoothK", m_SmoothK);
    m_Shader.SetInt("u_Repeat", m_Repeat);

    // 渲染
    m_Shader.SetInt("u_MaxSteps", m_MaxSteps);
    m_Shader.SetFloat("u_MaxDist", m_MaxDist);
    m_Shader.SetFloat("u_BoundingBox", m_BoundingBox);

    // 颜色
    m_Shader.SetVec3("u_StrutColor", glm::vec3(m_StrutColor[0], m_StrutColor[1], m_StrutColor[2]));
    m_Shader.SetVec3("u_NodeColor", glm::vec3(m_NodeColor[0], m_NodeColor[1], m_NodeColor[2]));
    m_Shader.SetVec3("u_BgColor", glm::vec3(m_BgColor[0], m_BgColor[1], m_BgColor[2]));

    glDepthMask(GL_FALSE);
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

void SDFLatticeDemo::OnImGui()
{
    const char *typeNames[] = {
        "Simple Cubic (SC)", "Body-Centered Cubic (BCC)",
        "Face-Centered Cubic (FCC)", "Octet Truss", "Kelvin Cell"
    };
    int typeIdx = static_cast<int>(m_Type);
    if (ImGui::Combo("Lattice Type", &typeIdx, typeNames, IM_ARRAYSIZE(typeNames)))
        m_Type = static_cast<SDFLatticeType>(typeIdx);

    ImGui::SliderFloat("Cell Size", &m_CellSize, 1.0f, 8.0f);
    ImGui::SliderFloat("Strut Radius", &m_StrutRadius, 0.02f, 0.5f);
    ImGui::SliderFloat("Node Radius", &m_NodeRadius, 0.0f, 0.6f,
                       "%.2f (0 = smooth-min only)");
    ImGui::SliderFloat("Smooth K", &m_SmoothK, 0.01f, 0.5f,
                       "%.3f (bigger = rounder joints)");
    ImGui::SliderInt("Repeat", &m_Repeat, 1, 6);

    ImGui::Separator();
    ImGui::Text("Rendering");
    ImGui::SliderInt("Max Steps", &m_MaxSteps, 32, 512);
    ImGui::SliderFloat("Max Distance", &m_MaxDist, 10.0f, 100.0f);
    ImGui::SliderFloat("Bounding Box", &m_BoundingBox, 2.0f, 30.0f);

    ImGui::Separator();
    ImGui::Text("Appearance");
    ImGui::ColorEdit3("Strut Color", m_StrutColor);
    ImGui::ColorEdit3("Node Color", m_NodeColor);
    ImGui::ColorEdit3("Background", m_BgColor);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 3.0f);
}

void SDFLatticeDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
