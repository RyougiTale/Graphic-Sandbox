#include "Demos/VoronoiSpongeDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include "Geometry/MarchingCubes.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <chrono>
#include <cmath>
#include <vector>

// ============================================================
// Worley 噪声 (Cellular Noise) — CPU 版
// ============================================================
//
// 与 Demo 6 shader 中的 voronoi3D 函数相同的算法, 但在 CPU 上执行:
//   1. 将空间划分为网格, 每个格子有一个种子点
//   2. 对于查询点 p, 搜索周围 3×3×3 = 27 个格子
//   3. 找到最近 (d1) 和次近 (d2) 种子点
//   4. SDF = d2 - d1 - thickness
//
// 时间复杂度: O(27) per sample (固定邻域大小)
// 这就是为什么 Worley 噪声能高效运行 — 不需要搜索所有种子点

static uint32_t pcgHash(uint32_t input)
{
    uint32_t state = input * 747796405u + 2891336453u;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

static float hashFloat(int x, int y, int z, int seed, int channel)
{
    uint32_t combined =
        static_cast<uint32_t>(x) * 374761393u +
        static_cast<uint32_t>(y) * 668265263u +
        static_cast<uint32_t>(z) * 1274126177u +
        static_cast<uint32_t>(seed) * 1013904223u +
        static_cast<uint32_t>(channel) * 1664525u;
    return static_cast<float>(pcgHash(combined)) / static_cast<float>(0xFFFFFFFFu);
}

// Worley 噪声: 返回 (d1, d2) — 到最近和次近种子的距离
static void worley3D(
    float px, float py, float pz,
    float cellScale, float jitter, int seed,
    float &outD1, float &outD2)
{
    // 将 p 映射到网格坐标
    float gx = px * cellScale;
    float gy = py * cellScale;
    float gz = pz * cellScale;

    int cx = static_cast<int>(std::floor(gx));
    int cy = static_cast<int>(std::floor(gy));
    int cz = static_cast<int>(std::floor(gz));

    float d1 = 1e10f, d2 = 1e10f;

    // 搜索 3×3×3 邻域
    for (int dz = -1; dz <= 1; dz++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
            {
                int nx = cx + dx;
                int ny = cy + dy;
                int nz = cz + dz;

                // 种子点 = 网格中心 + 随机偏移
                float sx = static_cast<float>(nx) + 0.5f +
                           (hashFloat(nx, ny, nz, seed, 0) - 0.5f) * jitter;
                float sy = static_cast<float>(ny) + 0.5f +
                           (hashFloat(nx, ny, nz, seed, 1) - 0.5f) * jitter;
                float sz = static_cast<float>(nz) + 0.5f +
                           (hashFloat(nx, ny, nz, seed, 2) - 0.5f) * jitter;

                float ddx = gx - sx;
                float ddy = gy - sy;
                float ddz = gz - sz;
                float dist = std::sqrt(ddx * ddx + ddy * ddy + ddz * ddz);

                if (dist < d1)
                {
                    d2 = d1;
                    d1 = dist;
                }
                else if (dist < d2)
                {
                    d2 = dist;
                }
            }

    // 将距离从网格空间转回世界空间
    float invScale = 1.0f / cellScale;
    outD1 = d1 * invScale;
    outD2 = d2 * invScale;
}

// ============================================================
// Init / Destroy
// ============================================================

void VoronoiSpongeDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/voronoi_sponge/sponge.vert",
                           "shaders/Demos/voronoi_sponge/sponge.frag");
    m_NeedRebuild = true;
}

void VoronoiSpongeDemo::OnDestroy()
{
    m_Mesh.Destroy();
}

// ============================================================
// Update / Render
// ============================================================

void VoronoiSpongeDemo::OnUpdate(float dt)
{
    if (m_AutoRotate)
        m_RotAngle += m_RotSpeed * dt;

    if (m_NeedRebuild)
    {
        Regenerate();
        m_NeedRebuild = false;
    }
}

void VoronoiSpongeDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid() || !m_Mesh.IsValid())
        return;

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

// ============================================================
// ImGui
// ============================================================

void VoronoiSpongeDemo::OnImGui()
{
    ImGui::Text("Voronoi Parameters");
    if (ImGui::SliderInt("Grid N", &m_GridN, 2, 6))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Jitter", &m_Jitter, 0.0f, 1.0f, "%.2f"))
        m_NeedRebuild = true;
    if (ImGui::SliderInt("Seed", &m_Seed, 0, 255))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Thickness", &m_Thickness, 0.02f, 0.5f))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Bounds", &m_BoundsSize, 3.0f, 12.0f))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::Text("Marching Cubes");
    if (ImGui::SliderInt("Resolution", &m_Resolution, 32, 128))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::Text("Appearance");
    ImGui::ColorEdit3("Front Color", m_Color);
    ImGui::ColorEdit3("Back Color", m_BackColor);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Rotation Speed", &m_RotSpeed, 0.0f, 3.0f);

    ImGui::Separator();
    ImGui::Text("Stats: %d tris, %d verts, %.1f ms build",
                m_TriCount, m_VertCount, m_BuildTimeMs);
}

// ============================================================
// Regenerate — Worley noise → Marching Cubes
// ============================================================

void VoronoiSpongeDemo::Regenerate()
{
    auto t0 = std::chrono::high_resolution_clock::now();

    float half = m_BoundsSize * 0.5f;
    float cellScale = static_cast<float>(m_GridN) / m_BoundsSize;

    // Voronoi SDF:
    //   d2 - d1 在边界处 = 0, 在细胞中心最大
    //   SDF = (d2 - d1) - thickness  →  < 0 为实体
    //
    // 与包围球相交: 只在 |p| < half 的范围内生成实体
    auto sdf = [&](float x, float y, float z) -> float
    {
        // 包围球裁剪
        float r = std::sqrt(x * x + y * y + z * z);
        float sphereDist = r - half;

        // Worley 噪声
        float d1, d2;
        worley3D(x, y, z, cellScale, m_Jitter, m_Seed, d1, d2);
        float voronoiDist = (d2 - d1) - m_Thickness;

        // 布尔交: max(sphereDist, voronoiDist)
        // 即: 只在球内保留泡沫结构
        return std::max(sphereDist, voronoiDist);
    };

    glm::vec3 gridMin(-half - 0.1f);
    glm::vec3 gridMax(half + 0.1f);

    auto result = MarchingCubes::Extract(sdf, gridMin, gridMax, m_Resolution, 0.0f);

    auto t1 = std::chrono::high_resolution_clock::now();
    m_BuildTimeMs = std::chrono::duration<float, std::milli>(t1 - t0).count();

    m_TriCount = static_cast<int>(result.indices.size() / 3);
    m_VertCount = static_cast<int>(result.vertices.size());

    // 上传到 GPU
    if (!result.vertices.empty())
    {
        m_Mesh.Upload(
            reinterpret_cast<const std::vector<MeshRenderer::MCVertex> &>(result.vertices),
            result.indices);
    }
}

void VoronoiSpongeDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
