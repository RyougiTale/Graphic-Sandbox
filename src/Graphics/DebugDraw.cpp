#include "Graphics/DebugDraw.h"
#include "Camera/ICamera.h"
#include <imgui.h>
#include <vector>

void DebugDraw::Init()
{
    m_Shader.LoadFromFiles("shaders/Graphics/debugdraw/debugdraw.vert",
                           "shaders/Graphics/debugdraw/debugdraw.frag");
    BuildAxesGeometry();
    BuildGridGeometry();
}

void DebugDraw::Shutdown()
{
    if (m_AxesVAO)
    {
        glDeleteVertexArrays(1, &m_AxesVAO);
        m_AxesVAO = 0;
    }
    if (m_AxesVBO)
    {
        glDeleteBuffers(1, &m_AxesVBO);
        m_AxesVBO = 0;
    }
    if (m_GridVAO)
    {
        glDeleteVertexArrays(1, &m_GridVAO);
        m_GridVAO = 0;
    }
    if (m_GridVBO)
    {
        glDeleteBuffers(1, &m_GridVBO);
        m_GridVBO = 0;
    }
}

void DebugDraw::Render(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid())
        return;

    m_Shader.Use();
    m_Shader.SetMat4("u_View", camera.GetViewMatrix());
    m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));

    // 先画 Grid, 再画 Axes, 这样坐标轴在上层
    if (m_ShowGrid)
    {
        glBindVertexArray(m_GridVAO);
        glDrawArrays(GL_LINES, 0, m_GridVertexCount);
        glBindVertexArray(0);
    }

    if (m_ShowAxes)
    {
        glLineWidth(2.0f);
        glBindVertexArray(m_AxesVAO);
        // 3条线, 每条2个顶点 = 6个顶点
        glDrawArrays(GL_LINES, 0, 6);
        glBindVertexArray(0);
        glLineWidth(1.0f);
    }
}

void DebugDraw::RenderImGui()
{
    ImGui::SetNextWindowPos(ImVec2(1330, 20), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Checkbox("Show Axes", &m_ShowAxes);
    ImGui::Checkbox("Show Grid", &m_ShowGrid);

    if (m_ShowGrid)
    {
        ImGui::Separator();
        bool gridChanged = ImGui::SliderInt("Grid Size", &m_GridSize, 1, 100);
        gridChanged |= ImGui::SliderFloat("Grid Spacing", &m_GridSpacing, 0.1f, 5.0f);
        gridChanged |= ImGui::ColorEdit3("Grid Color", m_GridColor);
        if (gridChanged)
        {
            RebuildGrid();
        }
    }

    ImGui::End();
}

// 坐标轴: 原点到各轴正方向, X=红, Y=绿, Z=蓝
void DebugDraw::BuildAxesGeometry()
{
    // 每个顶点: position(3) + color(3) = 6 floats
    // 3条线 x 2个顶点 = 6个顶点
    float vertices[] = {
        // X 轴 (红)
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        // Y 轴 (绿)
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        // Z 轴 (蓝)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_AxesVAO);
    glGenBuffers(1, &m_AxesVBO);

    glBindVertexArray(m_AxesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_AxesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// XZ 平面网格
void DebugDraw::BuildGridGeometry()
{
    std::vector<float> vertices;
    int N = m_GridSize;
    float spacing = m_GridSpacing;
    float r = m_GridColor[0], g = m_GridColor[1], b = m_GridColor[2];
    float halfSize = N * spacing;

    // Z 方向的线 (沿 X 轴排列)
    for (int i = -N; i <= N; i++)
    {
        float x = i * spacing;
        // 起点
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(-halfSize);
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
        // 终点
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(halfSize);
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }

    // X 方向的线 (沿 Z 轴排列)
    for (int i = -N; i <= N; i++)
    {
        float z = i * spacing;
        vertices.push_back(-halfSize);
        vertices.push_back(0.0f);
        vertices.push_back(z);
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);

        vertices.push_back(halfSize);
        vertices.push_back(0.0f);
        vertices.push_back(z);
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }

    // 每个顶点6个float, 总顶点数 = vertices.size() / 6
    // 3个position 3个RGB
    m_GridVertexCount = static_cast<int>(vertices.size()) / 6;

    glGenVertexArrays(1, &m_GridVAO);
    glGenBuffers(1, &m_GridVBO);

    glBindVertexArray(m_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
    // GL_DYNAMIC_DRAW: Grid 参数可通过 ImGui 调整
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ImGui 参数变化时重建 Grid 几何
void DebugDraw::RebuildGrid()
{
    if (m_GridVAO)
    {
        glDeleteVertexArrays(1, &m_GridVAO);
        m_GridVAO = 0;
    }
    if (m_GridVBO)
    {
        glDeleteBuffers(1, &m_GridVBO);
        m_GridVBO = 0;
    }
    BuildGridGeometry();
}
