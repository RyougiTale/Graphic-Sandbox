#include "Demos/VoronoiWireframeDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include "Geometry/ConvexPolytope.h"
#include "Geometry/Primitives.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <cmath>

// ============================================================
// 确定性哈希 — 用于种子点的可重复随机偏移
// ============================================================
// PCG (Permuted Congruential Generator) 的简化版本
// 输入: 网格坐标 + 用户种子 + 通道 → 输出: [0, 1) 浮点数
// 优点: 确定性 (同参数 → 同结果), 分布均匀, 无明显 pattern

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
    uint32_t h = pcgHash(combined);
    return static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
}

// ============================================================
// OpenGL 辅助: 实例属性绑定
// ============================================================

static void SetupMeshAttribs(GLuint vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // loc 0 = position (vec3), loc 1 = normal (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

static void SetupInstanceAttribs(GLuint instVBO)
{
    glBindBuffer(GL_ARRAY_BUFFER, instVBO);
    // loc 2,3,4 = 3 × vec3 per instance
    for (int i = 0; i < 3; i++)
    {
        glVertexAttribPointer(2 + i, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(glm::vec3),
                              reinterpret_cast<void *>(i * sizeof(glm::vec3)));
        glEnableVertexAttribArray(2 + i);
        glVertexAttribDivisor(2 + i, 1);
    }
}

// ============================================================
// Init / Destroy
// ============================================================

void VoronoiWireframeDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/voronoi_wireframe/wireframe.vert",
                           "shaders/Demos/voronoi_wireframe/wireframe.frag");

    // --- 球体模板 (joints + seeds 共用) ---
    {
        std::vector<float> verts;
        std::vector<unsigned int> indices;
        Primitives::GenerateSphere(verts, indices, 1.0f, 16, 12);
        m_SphereIndexCount = static_cast<int>(indices.size());

        glGenBuffers(1, &m_SphereVBO);
        glGenBuffers(1, &m_SphereEBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_SphereVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                     indices.data(), GL_STATIC_DRAW);
    }

    // Joint VAO
    glGenVertexArrays(1, &m_JointVAO);
    glGenBuffers(1, &m_JointInstVBO);
    glBindVertexArray(m_JointVAO);
    SetupMeshAttribs(m_SphereVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
    SetupInstanceAttribs(m_JointInstVBO);
    glBindVertexArray(0);

    // Seed VAO (同一球体 mesh, 不同实例数据)
    glGenVertexArrays(1, &m_SeedVAO);
    glGenBuffers(1, &m_SeedInstVBO);
    glBindVertexArray(m_SeedVAO);
    SetupMeshAttribs(m_SphereVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
    SetupInstanceAttribs(m_SeedInstVBO);
    glBindVertexArray(0);

    // --- 圆柱模板 (struts) ---
    {
        std::vector<float> verts;
        std::vector<unsigned int> indices;
        Primitives::GenerateCylinder(verts, indices, 1.0f, 8);
        m_CylIndexCount = static_cast<int>(indices.size());

        glGenBuffers(1, &m_CylVBO);
        glGenBuffers(1, &m_CylEBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_CylVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                     indices.data(), GL_STATIC_DRAW);
    }

    // Strut VAO
    glGenVertexArrays(1, &m_StrutVAO);
    glGenBuffers(1, &m_StrutInstVBO);
    glBindVertexArray(m_StrutVAO);
    SetupMeshAttribs(m_CylVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEBO);
    SetupInstanceAttribs(m_StrutInstVBO);
    glBindVertexArray(0);

    // --- 包围盒线框 ---
    glGenVertexArrays(1, &m_BoxVAO);
    glGenBuffers(1, &m_BoxVBO);

    m_NeedRebuild = true;
}

void VoronoiWireframeDemo::OnDestroy()
{
    auto delBuf = [](GLuint &b)
    { if (b) { glDeleteBuffers(1, &b); b = 0; } };
    auto delVAO = [](GLuint &v)
    { if (v) { glDeleteVertexArrays(1, &v); v = 0; } };

    delVAO(m_JointVAO);
    delBuf(m_JointInstVBO);
    delVAO(m_SeedVAO);
    delBuf(m_SeedInstVBO);
    delVAO(m_StrutVAO);
    delBuf(m_StrutInstVBO);
    delVAO(m_BoxVAO);
    delBuf(m_BoxVBO);
    delBuf(m_SphereVBO);
    delBuf(m_SphereEBO);
    delBuf(m_CylVBO);
    delBuf(m_CylEBO);
}

// ============================================================
// Update / Render
// ============================================================

void VoronoiWireframeDemo::OnUpdate(float dt)
{
    if (m_AutoRotate)
        m_RotAngle += m_RotSpeed * dt;

    if (m_NeedRebuild)
    {
        GenerateVoronoi();
        m_NeedRebuild = false;
    }
}

void VoronoiWireframeDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid())
        return;

    m_Shader.Use();

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_RotAngle, glm::vec3(0, 1, 0));
    m_Shader.SetMat4("u_Model", model);
    m_Shader.SetMat4("u_View", camera.GetViewMatrix());
    m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
    m_Shader.SetVec3("u_LightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.8f)));
    m_Shader.SetVec3("u_ViewPos", camera.GetPosition());

    glEnable(GL_DEPTH_TEST);

    // Struts (圆柱体 — Voronoi 边)
    if (m_ShowStruts && m_StrutCount > 0)
    {
        m_Shader.SetInt("u_RenderMode", 1);
        m_Shader.SetFloat("u_BondRadius", m_StrutRadius);
        glBindVertexArray(m_StrutVAO);
        glDrawElementsInstanced(GL_TRIANGLES, m_CylIndexCount,
                                GL_UNSIGNED_INT, nullptr, m_StrutCount);
    }

    // Joints (球体 — Voronoi 顶点)
    if (m_ShowJoints && m_JointCount > 0)
    {
        m_Shader.SetInt("u_RenderMode", 0);
        m_Shader.SetFloat("u_AtomRadius", m_JointRadius);
        glBindVertexArray(m_JointVAO);
        glDrawElementsInstanced(GL_TRIANGLES, m_SphereIndexCount,
                                GL_UNSIGNED_INT, nullptr, m_JointCount);
    }

    // Seeds (球体 — 种子点, 较大, 蓝色)
    if (m_ShowSeeds && m_SeedCount > 0)
    {
        m_Shader.SetInt("u_RenderMode", 0);
        m_Shader.SetFloat("u_AtomRadius", m_SeedRadius);
        glBindVertexArray(m_SeedVAO);
        glDrawElementsInstanced(GL_TRIANGLES, m_SphereIndexCount,
                                GL_UNSIGNED_INT, nullptr, m_SeedCount);
    }

    // Bounding box (线框)
    if (m_ShowBounds && m_BoxVertCount > 0)
    {
        m_Shader.SetInt("u_RenderMode", 2);
        glBindVertexArray(m_BoxVAO);
        glDrawArrays(GL_LINES, 0, m_BoxVertCount);
    }

    glBindVertexArray(0);
}

// ============================================================
// ImGui
// ============================================================

void VoronoiWireframeDemo::OnImGui()
{
    ImGui::Text("Seed Points");
    if (ImGui::SliderInt("Grid N", &m_GridN, 2, 6))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Jitter", &m_Jitter, 0.0f, 1.0f, "%.2f (0=grid, 1=random)"))
        m_NeedRebuild = true;
    if (ImGui::SliderInt("Seed", &m_Seed, 0, 255))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Bounds Size", &m_BoundsSize, 4.0f, 16.0f))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::Text("Geometry");
    ImGui::SliderFloat("Strut Radius", &m_StrutRadius, 0.01f, 0.2f);
    ImGui::SliderFloat("Joint Radius", &m_JointRadius, 0.02f, 0.3f);
    ImGui::SliderFloat("Seed Radius", &m_SeedRadius, 0.05f, 0.4f);

    ImGui::Separator();
    ImGui::Text("Visibility");
    ImGui::Checkbox("Show Struts", &m_ShowStruts);
    ImGui::Checkbox("Show Joints", &m_ShowJoints);
    ImGui::Checkbox("Show Seeds", &m_ShowSeeds);
    ImGui::Checkbox("Show Bounds", &m_ShowBounds);

    ImGui::Separator();
    ImGui::Text("Colors");
    if (ImGui::ColorEdit3("Strut Color", m_StrutColor))
        m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Joint Color", m_JointColor))
        m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Seed Color", m_SeedColor))
        m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Bounds Color", m_BoundsColor))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Rotation Speed", &m_RotSpeed, 0.0f, 3.0f);

    ImGui::Separator();
    ImGui::Text("Stats: %d seeds, %d vertices, %d edges",
                m_SeedCount, m_NumVertices, m_NumEdges);
}

// ============================================================
// Voronoi 构建
// ============================================================
//
// 流程:
//   1. 抖动网格 (Jittered Grid) 生成种子点
//      - 将空间分成 N×N×N 的网格
//      - 每个格子中心 + 随机偏移 → 种子点
//      - Jitter=0 → 规则网格 (所有 Voronoi cell 都是立方体)
//      - Jitter=1 → 完全随机 (自然泡沫效果)
//
//   2. 对每个种子, 用半平面裁剪构造 Voronoi 单元格
//      - 初始化为包围盒
//      - 对每个其他种子, 计算中垂面并裁剪
//      - 裁剪后得到凸多面体
//
//   3. 从所有单元格提取唯一边和顶点 (去重)
//
//   4. 构造实例数据, 上传到 GPU

void VoronoiWireframeDemo::GenerateVoronoi()
{
    m_JointInstances.clear();
    m_StrutInstances.clear();
    m_SeedInstances.clear();

    glm::vec3 strutCol(m_StrutColor[0], m_StrutColor[1], m_StrutColor[2]);
    glm::vec3 jointCol(m_JointColor[0], m_JointColor[1], m_JointColor[2]);
    glm::vec3 seedCol(m_SeedColor[0], m_SeedColor[1], m_SeedColor[2]);

    // ---- 1. 生成种子点 (抖动网格) ----
    std::vector<glm::vec3> seeds;
    float cellW = m_BoundsSize / static_cast<float>(m_GridN);
    float half = m_BoundsSize * 0.5f;

    for (int ix = 0; ix < m_GridN; ix++)
        for (int iy = 0; iy < m_GridN; iy++)
            for (int iz = 0; iz < m_GridN; iz++)
            {
                // 网格中心
                glm::vec3 base = glm::vec3(
                                     static_cast<float>(ix) + 0.5f,
                                     static_cast<float>(iy) + 0.5f,
                                     static_cast<float>(iz) + 0.5f) *
                                     cellW -
                                 glm::vec3(half);

                // 随机偏移 (Jitter)
                glm::vec3 jitter(
                    hashFloat(ix, iy, iz, m_Seed, 0) - 0.5f,
                    hashFloat(ix, iy, iz, m_Seed, 1) - 0.5f,
                    hashFloat(ix, iy, iz, m_Seed, 2) - 0.5f);
                seeds.push_back(base + jitter * m_Jitter * cellW);
            }

    // Seed 实例数据
    for (auto &s : seeds)
        m_SeedInstances.push_back({s, seedCol, glm::vec3(0)});

    // ---- 2. 对每个种子构建 Voronoi 单元格 ----
    const float eps = 1e-3f;
    std::vector<ConvexPolytope::Edge> allEdges;
    std::vector<glm::vec3> allVerts;

    // 去重辅助
    auto addUniqueEdge = [&](const glm::vec3 &a, const glm::vec3 &b)
    {
        for (auto &e : allEdges)
        {
            if ((glm::distance(e.a, a) < eps && glm::distance(e.b, b) < eps) ||
                (glm::distance(e.a, b) < eps && glm::distance(e.b, a) < eps))
                return;
        }
        allEdges.push_back({a, b});
    };

    auto addUniqueVert = [&](const glm::vec3 &v)
    {
        for (auto &ev : allVerts)
        {
            if (glm::distance(ev, v) < eps)
                return;
        }
        allVerts.push_back(v);
    };

    glm::vec3 bmin(-half), bmax(half);
    int numSeeds = static_cast<int>(seeds.size());

    for (int i = 0; i < numSeeds; i++)
    {
        ConvexPolytope cell = ConvexPolytope::MakeBox(bmin, bmax);

        for (int j = 0; j < numSeeds; j++)
        {
            if (i == j)
                continue;

            // 中垂面: 法线指向 j, 过 i-j 中点
            glm::vec3 diff = seeds[j] - seeds[i];
            float len = glm::length(diff);
            if (len < 1e-6f)
                continue;

            glm::vec3 normal = diff / len;
            glm::vec3 mid = (seeds[i] + seeds[j]) * 0.5f;
            float d = glm::dot(normal, mid);

            cell.ClipByPlane(normal, d);

            if (cell.IsEmpty())
                break;
        }

        // 从当前 cell 提取边和顶点
        auto edges = cell.ExtractEdges(eps);
        for (auto &e : edges)
        {
            addUniqueEdge(e.a, e.b);
            addUniqueVert(e.a);
            addUniqueVert(e.b);
        }
    }

    // ---- 3. 构造实例数据 ----
    for (auto &e : allEdges)
        m_StrutInstances.push_back({e.a, e.b, strutCol});

    for (auto &v : allVerts)
        m_JointInstances.push_back({v, jointCol, glm::vec3(0)});

    m_NumEdges = static_cast<int>(allEdges.size());
    m_NumVertices = static_cast<int>(allVerts.size());

    // ---- 4. 上传到 GPU ----
    m_SeedCount = static_cast<int>(m_SeedInstances.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_SeedInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_SeedInstances.size() * sizeof(InstanceData)),
                 m_SeedInstances.data(), GL_DYNAMIC_DRAW);

    m_JointCount = static_cast<int>(m_JointInstances.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_JointInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_JointInstances.size() * sizeof(InstanceData)),
                 m_JointInstances.data(), GL_DYNAMIC_DRAW);

    m_StrutCount = static_cast<int>(m_StrutInstances.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_StrutInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_StrutInstances.size() * sizeof(InstanceData)),
                 m_StrutInstances.data(), GL_DYNAMIC_DRAW);

    // ---- 包围盒线框 ----
    {
        std::vector<float> boxVerts;
        glm::vec3 bc(m_BoundsColor[0], m_BoundsColor[1], m_BoundsColor[2]);

        auto addLine = [&](glm::vec3 a, glm::vec3 b)
        {
            boxVerts.insert(boxVerts.end(), {a.x, a.y, a.z, bc.r, bc.g, bc.b});
            boxVerts.insert(boxVerts.end(), {b.x, b.y, b.z, bc.r, bc.g, bc.b});
        };

        glm::vec3 dx(m_BoundsSize, 0, 0);
        glm::vec3 dy(0, m_BoundsSize, 0);
        glm::vec3 dz(0, 0, m_BoundsSize);

        // 12 edges of the box
        addLine(bmin, bmin + dx);
        addLine(bmin, bmin + dy);
        addLine(bmin, bmin + dz);
        addLine(bmin + dx, bmin + dx + dy);
        addLine(bmin + dx, bmin + dx + dz);
        addLine(bmin + dy, bmin + dy + dx);
        addLine(bmin + dy, bmin + dy + dz);
        addLine(bmin + dz, bmin + dz + dx);
        addLine(bmin + dz, bmin + dz + dy);
        addLine(bmin + dx + dy, bmax);
        addLine(bmin + dx + dz, bmax);
        addLine(bmin + dy + dz, bmax);

        m_BoxVertCount = static_cast<int>(boxVerts.size() / 6);

        glBindVertexArray(m_BoxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_BoxVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(boxVerts.size() * sizeof(float)),
                     boxVerts.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
}

void VoronoiWireframeDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
