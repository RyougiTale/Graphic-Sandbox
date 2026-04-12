#include "Demos/MeshDualSmoothDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include "Geometry/IcoSphere.h"
#include "Geometry/MarchingCubes.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <chrono>
#include <unordered_map>
#include <cmath>
#include <algorithm>

// ============================================================
// SDF 基元
// ============================================================

// Capsule SDF: 线段 (a→b) 周围半径 r 的胶囊体
// 几何意义: 线段上最近点到 p 的距离 - r
static float sdCapsule(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, float r)
{
    glm::vec3 pa = p - a, ba = b - a;
    float h = glm::clamp(glm::dot(pa, ba) / glm::dot(ba, ba), 0.0f, 1.0f);
    return glm::length(pa - ba * h) - r;
}

// Smooth-Min: 在两个 SDF 相交处产生圆角过渡
// k 控制过渡宽度: k=0 → 普通 min, k>0 → 越来越圆滑
static float smin(float a, float b, float k)
{
    float h = glm::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return glm::mix(b, a, h) - k * h * (1.0f - h);
}

// ============================================================
// Init / Destroy
// ============================================================

void MeshDualSmoothDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/voronoi_sponge/sponge.vert",
                           "shaders/Demos/voronoi_sponge/sponge.frag");
    m_NeedRebuild = true;
}

void MeshDualSmoothDemo::OnDestroy()
{
    m_Mesh.Destroy();
}

void MeshDualSmoothDemo::OnUpdate(float dt)
{
    if (m_AutoRotate) m_RotAngle += m_RotSpeed * dt;
    if (m_NeedRebuild) { Regenerate(); m_NeedRebuild = false; }
}

void MeshDualSmoothDemo::OnRender(const ICamera &camera, float aspectRatio)
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

void MeshDualSmoothDemo::OnImGui()
{
    if (ImGui::SliderInt("Subdivisions", &m_Subdivisions, 0, 2))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Sphere Radius", &m_SphereRadius, 1.0f, 5.0f))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Strut Radius", &m_StrutRadius, 0.05f, 0.4f))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Smooth K", &m_SmoothK, 0.01f, 1.0f))
        m_NeedRebuild = true;
    if (ImGui::SliderInt("MC Resolution", &m_Resolution, 32, 128))
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
// 重建: 对偶图 → Capsule SDF → Smooth-Min → Marching Cubes
// ============================================================

void MeshDualSmoothDemo::Regenerate()
{
    auto t0 = std::chrono::high_resolution_clock::now();

    // 1. 生成 icosphere
    auto sphere = IcoSphere::Generate(m_Subdivisions, m_SphereRadius);
    int numTris = static_cast<int>(sphere.indices.size()) / 3;

    // 2. 计算重心
    std::vector<glm::vec3> bary(numTris);
    for (int i = 0; i < numTris; i++)
    {
        glm::vec3 a = sphere.vertices[sphere.indices[3 * i]];
        glm::vec3 b = sphere.vertices[sphere.indices[3 * i + 1]];
        glm::vec3 c = sphere.vertices[sphere.indices[3 * i + 2]];
        // 投影到球面
        bary[i] = glm::normalize((a + b + c) / 3.0f) * m_SphereRadius;
    }

    // 3. 构建邻接, 提取对偶边
    struct DualEdge { glm::vec3 a, b; };
    std::vector<DualEdge> edges;

    std::unordered_map<uint64_t, std::vector<int>> edgeMap;
    auto ek = [](uint32_t a, uint32_t b) -> uint64_t
    {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    };

    for (int i = 0; i < numTris; i++)
    {
        uint32_t v0 = sphere.indices[3 * i];
        uint32_t v1 = sphere.indices[3 * i + 1];
        uint32_t v2 = sphere.indices[3 * i + 2];
        edgeMap[ek(v0, v1)].push_back(i);
        edgeMap[ek(v1, v2)].push_back(i);
        edgeMap[ek(v2, v0)].push_back(i);
    }

    for (auto &[key, tris] : edgeMap)
    {
        if (tris.size() == 2)
            edges.push_back({bary[tris[0]], bary[tris[1]]});
    }

    // 4. 在体素网格上求值 SDF, 用 Marching Cubes 提取
    float bound = m_SphereRadius + m_StrutRadius + 0.5f;
    float strutR = m_StrutRadius;
    float smoothK = m_SmoothK;

    auto sdf = [&](float x, float y, float z) -> float
    {
        glm::vec3 p(x, y, z);
        float d = 1e10f;
        for (auto &e : edges)
            d = smin(d, sdCapsule(p, e.a, e.b, strutR), smoothK);
        return d;
    };

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

void MeshDualSmoothDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
