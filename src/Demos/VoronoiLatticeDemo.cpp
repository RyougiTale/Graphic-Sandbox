#include "Demos/VoronoiLatticeDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

void VoronoiLatticeDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/voronoi_lattice/voronoi_lattice.vert",
                           "shaders/Demos/voronoi_lattice/voronoi_lattice.frag");
    glGenVertexArrays(1, &m_VAO);
}

void VoronoiLatticeDemo::OnDestroy()
{
    if (m_VAO)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

void VoronoiLatticeDemo::OnUpdate(float deltaTime)
{
    if (m_AutoRotate)
        m_RotationAngle += m_RotationSpeed * deltaTime;
    if (m_Animate)
        m_Time += deltaTime * m_AnimSpeed;
}

void VoronoiLatticeDemo::OnRender(const ICamera &camera, float aspectRatio)
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
    m_Shader.SetMat4("u_InvRotation", glm::transpose(rotation));

    // Voronoi 参数
    m_Shader.SetInt("u_VoronoiType", static_cast<int>(m_Type));
    m_Shader.SetFloat("u_CellScale", m_CellScale);
    m_Shader.SetFloat("u_WallThickness", m_WallThickness);
    m_Shader.SetFloat("u_Randomness", m_Randomness);
    m_Shader.SetInt("u_Seed", m_Seed);
    m_Shader.SetFloat("u_Time", m_Time);
    m_Shader.SetInt("u_Animate", m_Animate ? 1 : 0);

    // 渲染
    m_Shader.SetInt("u_MaxSteps", m_MaxSteps);
    m_Shader.SetFloat("u_MaxDist", m_MaxDist);
    m_Shader.SetFloat("u_BoundingBox", m_BoundingBox);

    // 颜色
    m_Shader.SetVec3("u_WallColor", glm::vec3(m_WallColor[0], m_WallColor[1], m_WallColor[2]));
    m_Shader.SetVec3("u_CavityColor", glm::vec3(m_CavityColor[0], m_CavityColor[1], m_CavityColor[2]));
    m_Shader.SetVec3("u_BgColor", glm::vec3(m_BgColor[0], m_BgColor[1], m_BgColor[2]));

    glDepthMask(GL_FALSE);
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

void VoronoiLatticeDemo::OnImGui()
{
    const char *typeNames[] = {"Standard Foam", "Thick Wall", "Skeleton", "Smooth"};
    int typeIdx = static_cast<int>(m_Type);
    if (ImGui::Combo("Voronoi Type", &typeIdx, typeNames, IM_ARRAYSIZE(typeNames)))
        m_Type = static_cast<VoronoiType>(typeIdx);

    ImGui::SliderFloat("Cell Scale", &m_CellScale, 0.5f, 6.0f);
    ImGui::SliderFloat("Wall Thickness", &m_WallThickness, 0.01f, 0.5f);
    ImGui::SliderFloat("Randomness", &m_Randomness, 0.0f, 1.0f,
                       "%.2f (0=grid, 1=random)");
    ImGui::SliderInt("Seed", &m_Seed, 0, 255);

    ImGui::Separator();
    ImGui::Text("Rendering");
    ImGui::SliderInt("Max Steps", &m_MaxSteps, 32, 512);
    ImGui::SliderFloat("Max Distance", &m_MaxDist, 10.0f, 100.0f);
    ImGui::SliderFloat("Bounding Box", &m_BoundingBox, 2.0f, 30.0f);

    ImGui::Separator();
    ImGui::Text("Appearance");
    ImGui::ColorEdit3("Wall Color", m_WallColor);
    ImGui::ColorEdit3("Cavity Color", m_CavityColor);
    ImGui::ColorEdit3("Background", m_BgColor);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 3.0f);
    ImGui::Checkbox("Animate Seeds", &m_Animate);
    if (m_Animate)
        ImGui::SliderFloat("Anim Speed", &m_AnimSpeed, 0.0f, 2.0f);
}

void VoronoiLatticeDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
