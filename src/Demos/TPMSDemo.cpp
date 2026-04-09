#include "Demos/TPMSDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <cmath>

void TPMSDemo::OnInit()
{
    // 全屏三角形 + ray marching fragment shader
    m_Shader.LoadFromFiles("shaders/Demos/tpms/tpms.vert",
                           "shaders/Demos/tpms/tpms.frag");
    glGenVertexArrays(1, &m_VAO);
}

void TPMSDemo::OnDestroy()
{
    if (m_VAO)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

void TPMSDemo::OnUpdate(float deltaTime)
{
    if (m_Animate)
        m_Time += deltaTime * m_AnimSpeed;
}

void TPMSDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid())
        return;

    m_Shader.Use();

    // 相机参数 — fragment shader 需要从像素坐标重建射线
    m_Shader.SetMat4("u_InvView", glm::inverse(camera.GetViewMatrix()));
    m_Shader.SetMat4("u_InvProjection", glm::inverse(camera.GetProjectionMatrix(aspectRatio)));
    m_Shader.SetVec3("u_CameraPos", camera.GetPosition());

    // TPMS 参数
    float k = 2.0f * glm::pi<float>() / m_CellSize;
    m_Shader.SetInt("u_TPMSType", static_cast<int>(m_Type));
    m_Shader.SetInt("u_SolidMode", static_cast<int>(m_SolidMode));
    m_Shader.SetFloat("u_K", k);
    m_Shader.SetFloat("u_Thickness", m_Thickness);
    m_Shader.SetFloat("u_IsoValue", m_IsoValue);
    m_Shader.SetFloat("u_Time", m_Time);
    m_Shader.SetInt("u_Animate", m_Animate ? 1 : 0);

    // 渲染参数
    m_Shader.SetInt("u_MaxSteps", m_MaxSteps);
    m_Shader.SetFloat("u_MaxDist", m_MaxDist);
    m_Shader.SetFloat("u_BoundingBox", m_BoundingBox);

    // 颜色
    m_Shader.SetVec3("u_Color1", glm::vec3(m_Color1[0], m_Color1[1], m_Color1[2]));
    m_Shader.SetVec3("u_Color2", glm::vec3(m_Color2[0], m_Color2[1], m_Color2[2]));
    m_Shader.SetVec3("u_BgColor", glm::vec3(m_BgColor[0], m_BgColor[1], m_BgColor[2]));

    // 关闭深度写入 (全屏 quad, 深度由 ray marching 自行处理)
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
}

void TPMSDemo::OnImGui()
{
    const char *typeNames[] = {"Gyroid", "Schwarz P", "Schwarz D", "Neovius", "Lidinoid"};
    int typeIdx = static_cast<int>(m_Type);
    if (ImGui::Combo("TPMS Type", &typeIdx, typeNames, IM_ARRAYSIZE(typeNames)))
        m_Type = static_cast<TPMSType>(typeIdx);

    const char *modeNames[] = {"Sheet |F| < t", "Network F < t"};
    int modeIdx = static_cast<int>(m_SolidMode);
    if (ImGui::Combo("Solid Mode", &modeIdx, modeNames, IM_ARRAYSIZE(modeNames)))
        m_SolidMode = static_cast<TPMSSolidMode>(modeIdx);

    ImGui::SliderFloat("Cell Size", &m_CellSize, 0.5f, 10.0f);
    ImGui::SliderFloat("Thickness", &m_Thickness, 0.01f, 1.0f);
    ImGui::SliderFloat("Iso Value", &m_IsoValue, -1.5f, 1.5f);

    ImGui::Separator();
    ImGui::Text("Rendering");
    ImGui::SliderInt("Max Steps", &m_MaxSteps, 32, 512);
    ImGui::SliderFloat("Max Distance", &m_MaxDist, 10.0f, 100.0f);
    ImGui::SliderFloat("Bounding Box", &m_BoundingBox, 2.0f, 30.0f);

    ImGui::Separator();
    ImGui::Text("Appearance");
    ImGui::ColorEdit3("Front Color", m_Color1);
    ImGui::ColorEdit3("Back Color", m_Color2);
    ImGui::ColorEdit3("Background", m_BgColor);

    ImGui::Separator();
    ImGui::Checkbox("Animate", &m_Animate);
    if (m_Animate)
        ImGui::SliderFloat("Anim Speed", &m_AnimSpeed, 0.0f, 3.0f);
}

void TPMSDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
