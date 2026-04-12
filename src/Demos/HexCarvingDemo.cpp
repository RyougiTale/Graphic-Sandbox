#include "Demos/HexCarvingDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include "Geometry/MarchingCubes.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <chrono>
#include <cmath>
#include <algorithm>

// ============================================================
// 2D 六角距离场 — 轴向坐标系 (Axial Coordinates)
// ============================================================
//
// 六角网格的坐标系统:
//   正方形网格用 (x, y) 直角坐标
//   六角网格用轴向坐标 (q, r):
//     q 沿水平方向, r 沿 60° 方向
//     第三个坐标 s = -q - r (立方体坐标约束)
//
// 找到最近六角中心的算法:
//   1. 将笛卡尔坐标转换为连续轴向坐标
//   2. 四舍五入到最近整数 (cube rounding)
//   3. 修正舍入误差 (确保 q + r + s = 0)
//   4. 转回笛卡尔坐标得到六角中心位置
//
// 时间复杂度: O(1) — 不需要搜索, 直接计算

static float hexDist2D(float px, float py, float cellSize)
{
    // 归一化到单位网格
    float x = px / cellSize;
    float y = py / cellSize;

    // 笛卡尔 → 轴向坐标
    float q = (2.0f / 3.0f) * x;
    float r = (-1.0f / 3.0f) * x + (std::sqrt(3.0f) / 3.0f) * y;
    float s = -q - r;

    // Cube rounding: 四舍五入到最近六角中心
    float qi = std::round(q), ri = std::round(r), si = std::round(s);
    float qd = std::abs(qi - q), rd = std::abs(ri - r), sd = std::abs(si - s);

    // 修正: 舍入误差最大的坐标由另外两个决定
    if (qd > rd && qd > sd)
        qi = -ri - si;
    else if (rd > sd)
        ri = -qi - si;

    // 轴向坐标 → 笛卡尔 (六角中心位置)
    float cx = cellSize * 1.5f * qi;
    float cy = cellSize * std::sqrt(3.0f) * (ri + qi * 0.5f);

    float dx = px - cx;
    float dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}

// ============================================================
// 三平面投影 + CSG 布尔差
// ============================================================
//
// 三平面投影 (Triplanar Mapping):
//   从三个正交方向 (XY, YZ, XZ) 投影 2D pattern
//   取并集 (min): 任何一个方向有孔洞 → 结果有孔洞
//
// CSG 布尔差:
//   result = max(shell, -holes)
//   shell < 0 的区域是实体
//   holes < 0 的区域是孔洞
//   max(shell, -holes): 从实体中挖去孔洞

static float shellSDF(float x, float y, float z, float radius, float thickness)
{
    float r = std::sqrt(x * x + y * y + z * z);
    return std::abs(r - radius) - thickness * 0.5f;
}

static float triplanarHexHoles(float x, float y, float z, float cellSize, float holeRadius)
{
    // 三个正交平面上的六角孔洞
    float dxy = hexDist2D(x, y, cellSize) - holeRadius;
    float dyz = hexDist2D(y, z, cellSize) - holeRadius;
    float dxz = hexDist2D(x, z, cellSize) - holeRadius;
    return std::min({dxy, dyz, dxz}); // 并集: 任一平面有孔 → 整体有孔
}

// ============================================================
// Init / Destroy
// ============================================================

void HexCarvingDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/voronoi_sponge/sponge.vert",
                           "shaders/Demos/voronoi_sponge/sponge.frag");
    m_NeedRebuild = true;
}

void HexCarvingDemo::OnDestroy()
{
    m_Mesh.Destroy();
}

void HexCarvingDemo::OnUpdate(float dt)
{
    if (m_AutoRotate) m_RotAngle += m_RotSpeed * dt;
    if (m_NeedRebuild) { Regenerate(); m_NeedRebuild = false; }
}

void HexCarvingDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid() || !m_Mesh.IsValid()) return;

    m_Shader.Use();
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_RotAngle, glm::vec3(0, 1, 0));
    m_Shader.SetMat4("u_Model", model);
    m_Shader.SetMat4("u_View", camera.GetViewMatrix());
    m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
    m_Shader.SetVec3("u_LightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.8f)));
    m_Shader.SetVec3("u_ViewPos", camera.GetPosition());
    m_Shader.SetVec3("u_Color", glm::vec3(m_Color[0], m_Color[1], m_Color[2]));
    m_Shader.SetVec3("u_BackColor", glm::vec3(m_BackColor[0], m_BackColor[1], m_BackColor[2]));

    glEnable(GL_DEPTH_TEST);
    m_Mesh.Draw();
}

void HexCarvingDemo::OnImGui()
{
    ImGui::Text("Shell");
    if (ImGui::SliderFloat("Shell Radius", &m_ShellRadius, 1.0f, 5.0f))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Shell Thickness", &m_ShellThickness, 0.1f, 1.0f))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::Text("Hex Pattern");
    if (ImGui::SliderFloat("Hex Cell Size", &m_HexCellSize, 0.3f, 3.0f))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Hole Radius", &m_HoleRadius, 0.1f, m_HexCellSize * 0.8f))
        m_NeedRebuild = true;

    ImGui::Separator();
    if (ImGui::SliderInt("MC Resolution", &m_Resolution, 48, 128))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::ColorEdit3("Front Color", m_Color);
    ImGui::ColorEdit3("Back Color", m_BackColor);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate) ImGui::SliderFloat("Speed", &m_RotSpeed, 0.0f, 3.0f);

    ImGui::Separator();
    ImGui::Text("Stats: %d tris, %.1f ms build", m_TriCount, m_BuildTimeMs);
}

// ============================================================
// Regenerate — Shell SDF − Triplanar Hex → Marching Cubes
// ============================================================

void HexCarvingDemo::Regenerate()
{
    auto t0 = std::chrono::high_resolution_clock::now();

    float R = m_ShellRadius;
    float thick = m_ShellThickness;
    float cellSz = m_HexCellSize;
    float holeR = m_HoleRadius;

    auto sdf = [&](float x, float y, float z) -> float
    {
        float shell = shellSDF(x, y, z, R, thick);
        float holes = triplanarHexHoles(x, y, z, cellSz, holeR);
        // CSG 布尔差: 从壳体中减去孔洞
        return std::max(shell, -holes);
    };

    float bound = R + thick + 0.5f;
    auto result = MarchingCubes::Extract(sdf,
                                         glm::vec3(-bound), glm::vec3(bound),
                                         m_Resolution);

    auto t1 = std::chrono::high_resolution_clock::now();
    m_BuildTimeMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    m_TriCount = static_cast<int>(result.indices.size()) / 3;

    if (!result.vertices.empty())
    {
        m_Mesh.Upload(
            reinterpret_cast<const std::vector<MeshRenderer::MCVertex> &>(result.vertices),
            result.indices);
    }
}

void HexCarvingDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
