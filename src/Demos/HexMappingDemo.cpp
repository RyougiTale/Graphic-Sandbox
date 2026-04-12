#include "Demos/HexMappingDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include "Geometry/IcoSphere.h"
#include "Geometry/Primitives.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <cmath>
#include <vector>

// ============================================================
// OpenGL 辅助
// ============================================================

static void SetupMeshAttribs(GLuint vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

static void SetupInstAttribs(GLuint instVBO)
{
    glBindBuffer(GL_ARRAY_BUFFER, instVBO);
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
// UV 参数化 + 重心坐标插值
// ============================================================

// 球面 UV 映射: 3D 点 → (u, v) ∈ [0, 1]²
static glm::vec2 sphericalUV(const glm::vec3 &pos)
{
    glm::vec3 n = glm::normalize(pos);
    float u = std::atan2(n.z, n.x) / (2.0f * glm::pi<float>()) + 0.5f;
    float v = std::asin(glm::clamp(n.y, -1.0f, 1.0f)) / glm::pi<float>() + 0.5f;
    return {u, v};
}

// UV 空间中的三角形, 附带 3D 位置用于插值
struct UVTriangle
{
    glm::vec2 uv[3];
    glm::vec3 pos[3];
};

// 重心坐标: 判断点 p 是否在三角形 (a, b, c) 内
// 如果在内, 返回 true 并输出重心坐标 (lambda1=a权重, lambda2=b权重, lambda3=c权重)
static bool barycentricCoords(
    glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c,
    float &l1, float &l2, float &l3)
{
    glm::vec2 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d02 = glm::dot(v0, v2);
    float d11 = glm::dot(v1, v1);
    float d12 = glm::dot(v1, v2);

    float denom = d00 * d11 - d01 * d01;
    if (std::abs(denom) < 1e-10f)
        return false;

    float inv = 1.0f / denom;
    l2 = (d11 * d02 - d01 * d12) * inv; // b 的权重
    l3 = (d00 * d12 - d01 * d02) * inv; // c 的权重
    l1 = 1.0f - l2 - l3;                // a 的权重

    const float eps = -0.01f;
    return (l1 >= eps && l2 >= eps && l3 >= eps);
}

// 在 UV 三角形列表中查找点 p, 返回插值后的 3D 位置
static bool mapUVto3D(
    const glm::vec2 &uvPoint,
    const std::vector<UVTriangle> &triangles,
    glm::vec3 &outPos)
{
    for (auto &tri : triangles)
    {
        float l1, l2, l3;
        if (barycentricCoords(uvPoint, tri.uv[0], tri.uv[1], tri.uv[2], l1, l2, l3))
        {
            outPos = l1 * tri.pos[0] + l2 * tri.pos[1] + l3 * tri.pos[2];
            return true;
        }
    }
    return false;
}

// ============================================================
// Init / Destroy
// ============================================================

void HexMappingDemo::OnInit()
{
    m_WireShader.LoadFromFiles("shaders/Demos/voronoi_wireframe/wireframe.vert",
                               "shaders/Demos/voronoi_wireframe/wireframe.frag");
    m_SurfShader.LoadFromFiles("shaders/Demos/voronoi_sponge/sponge.vert",
                               "shaders/Demos/voronoi_sponge/sponge.frag");

    // Sphere template
    {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        Primitives::GenerateSphere(v, idx, 1.0f, 10, 6);
        m_SphereIdxCount = static_cast<int>(idx.size());
        glGenBuffers(1, &m_SphereVBO);
        glGenBuffers(1, &m_SphereEBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_SphereVBO);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    }

    // Cylinder template
    {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        Primitives::GenerateCylinder(v, idx, 1.0f, 6);
        m_CylIdxCount = static_cast<int>(idx.size());
        glGenBuffers(1, &m_CylVBO);
        glGenBuffers(1, &m_CylEBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_CylVBO);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    }

    glGenVertexArrays(1, &m_JointVAO);
    glGenBuffers(1, &m_JointInstVBO);
    glBindVertexArray(m_JointVAO);
    SetupMeshAttribs(m_SphereVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
    SetupInstAttribs(m_JointInstVBO);
    glBindVertexArray(0);

    glGenVertexArrays(1, &m_StrutVAO);
    glGenBuffers(1, &m_StrutInstVBO);
    glBindVertexArray(m_StrutVAO);
    SetupMeshAttribs(m_CylVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEBO);
    SetupInstAttribs(m_StrutInstVBO);
    glBindVertexArray(0);

    m_NeedRebuild = true;
}

void HexMappingDemo::OnDestroy()
{
    auto del = [](GLuint &b) { if (b) { glDeleteBuffers(1, &b); b = 0; } };
    auto delV = [](GLuint &v) { if (v) { glDeleteVertexArrays(1, &v); v = 0; } };
    delV(m_JointVAO); del(m_JointInstVBO);
    delV(m_StrutVAO); del(m_StrutInstVBO);
    del(m_SphereVBO); del(m_SphereEBO);
    del(m_CylVBO); del(m_CylEBO);
    m_BaseMesh.Destroy();
}

void HexMappingDemo::OnUpdate(float dt)
{
    if (m_AutoRotate) m_RotAngle += m_RotSpeed * dt;
    if (m_NeedRebuild) { Regenerate(); m_NeedRebuild = false; }
}

void HexMappingDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_RotAngle, glm::vec3(0, 1, 0));
    glEnable(GL_DEPTH_TEST);

    // Base sphere
    if (m_ShowBaseMesh && m_BaseMesh.IsValid())
    {
        m_SurfShader.Use();
        m_SurfShader.SetMat4("u_Model", model);
        m_SurfShader.SetMat4("u_View", camera.GetViewMatrix());
        m_SurfShader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
        m_SurfShader.SetVec3("u_LightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.8f)));
        m_SurfShader.SetVec3("u_ViewPos", camera.GetPosition());
        m_SurfShader.SetVec3("u_Color", glm::vec3(m_MeshColor[0], m_MeshColor[1], m_MeshColor[2]));
        m_SurfShader.SetVec3("u_BackColor", glm::vec3(m_MeshColor[0], m_MeshColor[1], m_MeshColor[2]) * 0.5f);
        if (m_BaseMeshWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        m_BaseMesh.Draw();
        if (m_BaseMeshWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Hex wireframe
    if (m_ShowHex && m_WireShader.IsValid())
    {
        m_WireShader.Use();
        m_WireShader.SetMat4("u_Model", model);
        m_WireShader.SetMat4("u_View", camera.GetViewMatrix());
        m_WireShader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
        m_WireShader.SetVec3("u_LightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.8f)));
        m_WireShader.SetVec3("u_ViewPos", camera.GetPosition());

        if (m_StrutCount > 0)
        {
            m_WireShader.SetInt("u_RenderMode", 1);
            m_WireShader.SetFloat("u_BondRadius", m_StrutRadius);
            glBindVertexArray(m_StrutVAO);
            glDrawElementsInstanced(GL_TRIANGLES, m_CylIdxCount, GL_UNSIGNED_INT, nullptr, m_StrutCount);
        }
        if (m_JointCount > 0)
        {
            m_WireShader.SetInt("u_RenderMode", 0);
            m_WireShader.SetFloat("u_AtomRadius", m_JointRadius);
            glBindVertexArray(m_JointVAO);
            glDrawElementsInstanced(GL_TRIANGLES, m_SphereIdxCount, GL_UNSIGNED_INT, nullptr, m_JointCount);
        }
        glBindVertexArray(0);
    }
}

void HexMappingDemo::OnImGui()
{
    if (ImGui::SliderInt("Subdivisions", &m_Subdivisions, 2, 4))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Sphere Radius", &m_SphereRadius, 1.0f, 6.0f))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Hex Size", &m_HexSize, 0.03f, 0.2f, "%.3f"))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::SliderFloat("Strut Radius", &m_StrutRadius, 0.005f, 0.08f);
    ImGui::SliderFloat("Joint Radius", &m_JointRadius, 0.01f, 0.1f);

    ImGui::Separator();
    ImGui::Checkbox("Show Hex Grid", &m_ShowHex);
    ImGui::Checkbox("Show Base Mesh", &m_ShowBaseMesh);
    ImGui::Checkbox("Wireframe", &m_BaseMeshWireframe);

    ImGui::Separator();
    if (ImGui::ColorEdit3("Hex Color", m_StrutColor)) m_NeedRebuild = true;
    ImGui::ColorEdit3("Mesh Color", m_MeshColor);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate) ImGui::SliderFloat("Speed", &m_RotSpeed, 0.0f, 3.0f);

    ImGui::Separator();
    ImGui::Text("Stats: %d hex verts, %d hex edges", m_HexVertCount, m_HexEdgeCount);
}

// ============================================================
// 六角网格生成 + UV→3D 映射
// ============================================================

void HexMappingDemo::Regenerate()
{
    m_JointInst.clear();
    m_StrutInst.clear();

    glm::vec3 sCol(m_StrutColor[0], m_StrutColor[1], m_StrutColor[2]);
    glm::vec3 jCol(m_JointColor[0], m_JointColor[1], m_JointColor[2]);

    // 1. 生成 icosphere
    auto sphere = IcoSphere::Generate(m_Subdivisions, m_SphereRadius);

    // Upload base mesh
    {
        std::vector<float> md;
        for (auto &v : sphere.vertices)
        {
            glm::vec3 n = glm::normalize(v);
            md.insert(md.end(), {v.x, v.y, v.z, n.x, n.y, n.z});
        }
        m_BaseMesh.Upload(md, sphere.indices);
    }

    // 2. 计算 UV 并构建 UV 三角形列表
    std::vector<glm::vec2> uvs(sphere.vertices.size());
    for (size_t i = 0; i < sphere.vertices.size(); i++)
        uvs[i] = sphericalUV(sphere.vertices[i]);

    std::vector<UVTriangle> uvTris;
    for (size_t i = 0; i < sphere.indices.size(); i += 3)
    {
        UVTriangle tri;
        for (int k = 0; k < 3; k++)
        {
            uint32_t vi = sphere.indices[i + k];
            tri.uv[k] = uvs[vi];
            tri.pos[k] = sphere.vertices[vi];
        }

        // 修复 UV 接缝: 如果 u 坐标差距 > 0.5, 调整较小的 u 值
        // 这是球面 UV 的标准处理: atan2 在 ±π 处不连续
        float maxU = std::max({tri.uv[0].x, tri.uv[1].x, tri.uv[2].x});
        float minU = std::min({tri.uv[0].x, tri.uv[1].x, tri.uv[2].x});
        if (maxU - minU > 0.5f)
        {
            for (int k = 0; k < 3; k++)
            {
                if (tri.uv[k].x < 0.3f)
                    tri.uv[k].x += 1.0f;
            }
        }

        uvTris.push_back(tri);
    }

    // 3. 生成六角网格在 UV 空间中
    // 使用 "pointy-top" 六角形布局
    float hexR = m_HexSize;
    float rowH = hexR * std::sqrt(3.0f);
    float colW = hexR * 1.5f;

    // 六角顶点 + 3D 位置
    struct HexVert
    {
        glm::vec2 uv;
        glm::vec3 pos3d;
        bool valid;
    };

    // 为避免重复, 用网格索引缓存六角顶点
    // key = (col, row, corner_index)
    struct HexKey
    {
        int col, row, corner;
        bool operator==(const HexKey &o) const
        {
            return col == o.col && row == o.row && corner == o.corner;
        }
    };
    struct HexKeyHash
    {
        size_t operator()(const HexKey &k) const
        {
            return std::hash<int>()(k.col * 10000 + k.row * 100 + k.corner);
        }
    };

    // 生成所有六角中心
    struct HexCell { int col, row; glm::vec2 center; };
    std::vector<HexCell> cells;

    int numCols = static_cast<int>(1.0f / colW) + 2;
    int numRows = static_cast<int>(1.0f / rowH) + 2;

    for (int c = -1; c <= numCols; c++)
    {
        float offset = (((c % 2) + 2) % 2 == 1) ? rowH * 0.5f : 0.0f;
        for (int r = -1; r <= numRows; r++)
        {
            glm::vec2 center(c * colW, r * rowH + offset);
            // 只保留 UV 空间 [0,1]² 附近的六角形
            if (center.x >= -hexR * 2 && center.x <= 1.0f + hexR * 2 &&
                center.y >= -hexR * 2 && center.y <= 1.0f + hexR * 2)
            {
                cells.push_back({c, r, center});
            }
        }
    }

    // 4. 对每个六角形, 生成 6 个顶点并映射到 3D
    // 六角顶点: center + hexR * (cos(60°*k + 30°), sin(60°*k + 30°))
    // 用 "flat-top" 偏移 30° 使边水平对齐

    // 收集所有唯一的 3D 顶点和边
    std::vector<glm::vec3> allVerts3D;
    std::vector<std::pair<int, int>> allEdges;

    // 用 UV 坐标 (量化到网格) 去重顶点
    auto quantize = [](glm::vec2 uv) -> uint64_t
    {
        int32_t u = static_cast<int32_t>(uv.x * 100000.0f);
        int32_t v = static_cast<int32_t>(uv.y * 100000.0f);
        return (static_cast<uint64_t>(static_cast<uint32_t>(u)) << 32) |
               static_cast<uint32_t>(v);
    };

    std::unordered_map<uint64_t, int> uvToIdx;

    auto getOrCreateVertex = [&](glm::vec2 uvPt) -> int
    {
        uint64_t key = quantize(uvPt);
        auto it = uvToIdx.find(key);
        if (it != uvToIdx.end())
            return it->second;

        glm::vec3 pos3d;
        // 尝试在原始 UV 范围 [0,1] 中查找
        glm::vec2 queryUV = uvPt;
        // 处理超出 [0,1] 的 UV (因为六角形可能跨越边界)
        if (queryUV.x > 1.0f) queryUV.x -= 1.0f;
        if (queryUV.x < 0.0f) queryUV.x += 1.0f;

        bool found = mapUVto3D(uvPt, uvTris, pos3d);
        if (!found)
        {
            // 尝试偏移后的 UV
            glm::vec2 shifted = uvPt;
            if (shifted.x > 1.0f) shifted.x -= 1.0f;
            found = mapUVto3D(shifted, uvTris, pos3d);
        }

        if (!found || queryUV.y < 0.02f || queryUV.y > 0.98f)
        {
            // 极点附近或找不到: 标记为无效
            uvToIdx[key] = -1;
            return -1;
        }

        int idx = static_cast<int>(allVerts3D.size());
        allVerts3D.push_back(pos3d);
        uvToIdx[key] = idx;
        return idx;
    };

    for (auto &cell : cells)
    {
        int cornerIdx[6];
        bool allValid = true;

        for (int k = 0; k < 6; k++)
        {
            float angle = glm::pi<float>() / 3.0f * static_cast<float>(k) + glm::pi<float>() / 6.0f;
            glm::vec2 cornerUV = cell.center + hexR * glm::vec2(std::cos(angle), std::sin(angle));
            cornerIdx[k] = getOrCreateVertex(cornerUV);
            if (cornerIdx[k] < 0) allValid = false;
        }

        // 只为有效的六角形添加边
        if (allValid)
        {
            for (int k = 0; k < 6; k++)
            {
                int a = cornerIdx[k];
                int b = cornerIdx[(k + 1) % 6];
                // 去重 (只添加 a < b 的边)
                int lo = std::min(a, b), hi = std::max(a, b);
                bool dup = false;
                for (auto &e : allEdges)
                {
                    if (e.first == lo && e.second == hi) { dup = true; break; }
                }
                if (!dup)
                    allEdges.push_back({lo, hi});
            }
        }
    }

    // 5. 构建实例数据
    for (auto &e : allEdges)
        m_StrutInst.push_back({allVerts3D[e.first], allVerts3D[e.second], sCol});

    for (auto &v : allVerts3D)
        m_JointInst.push_back({v, jCol, glm::vec3(0)});

    m_HexEdgeCount = static_cast<int>(allEdges.size());
    m_HexVertCount = static_cast<int>(allVerts3D.size());

    // Upload
    m_JointCount = static_cast<int>(m_JointInst.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_JointInstVBO);
    glBufferData(GL_ARRAY_BUFFER, m_JointInst.size() * sizeof(InstanceData),
                 m_JointInst.data(), GL_DYNAMIC_DRAW);

    m_StrutCount = static_cast<int>(m_StrutInst.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_StrutInstVBO);
    glBufferData(GL_ARRAY_BUFFER, m_StrutInst.size() * sizeof(InstanceData),
                 m_StrutInst.data(), GL_DYNAMIC_DRAW);
}

void HexMappingDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_WireShader);
    hotReload.Watch(&m_SurfShader);
}
