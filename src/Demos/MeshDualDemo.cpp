#include "Demos/MeshDualDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include "Geometry/IcoSphere.h"
#include "Geometry/Primitives.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <unordered_map>
#include <algorithm>

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
// Init / Destroy
// ============================================================

void MeshDualDemo::OnInit()
{
    m_WireShader.LoadFromFiles("shaders/Demos/voronoi_wireframe/wireframe.vert",
                               "shaders/Demos/voronoi_wireframe/wireframe.frag");
    m_SurfShader.LoadFromFiles("shaders/Demos/voronoi_sponge/sponge.vert",
                               "shaders/Demos/voronoi_sponge/sponge.frag");

    // Sphere template
    {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        Primitives::GenerateSphere(v, idx, 1.0f, 12, 8);
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

    // Joint VAO
    glGenVertexArrays(1, &m_JointVAO);
    glGenBuffers(1, &m_JointInstVBO);
    glBindVertexArray(m_JointVAO);
    SetupMeshAttribs(m_SphereVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
    SetupInstAttribs(m_JointInstVBO);
    glBindVertexArray(0);

    // Strut VAO
    glGenVertexArrays(1, &m_StrutVAO);
    glGenBuffers(1, &m_StrutInstVBO);
    glBindVertexArray(m_StrutVAO);
    SetupMeshAttribs(m_CylVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEBO);
    SetupInstAttribs(m_StrutInstVBO);
    glBindVertexArray(0);

    m_NeedRebuild = true;
}

void MeshDualDemo::OnDestroy()
{
    auto del = [](GLuint &b) { if (b) { glDeleteBuffers(1, &b); b = 0; } };
    auto delV = [](GLuint &v) { if (v) { glDeleteVertexArrays(1, &v); v = 0; } };
    delV(m_JointVAO); del(m_JointInstVBO);
    delV(m_StrutVAO); del(m_StrutInstVBO);
    del(m_SphereVBO); del(m_SphereEBO);
    del(m_CylVBO); del(m_CylEBO);
    m_BaseMesh.Destroy();
}

// ============================================================
// Update / Render
// ============================================================

void MeshDualDemo::OnUpdate(float dt)
{
    if (m_AutoRotate) m_RotAngle += m_RotSpeed * dt;
    if (m_NeedRebuild) { Regenerate(); m_NeedRebuild = false; }
}

void MeshDualDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_RotAngle, glm::vec3(0, 1, 0));
    glEnable(GL_DEPTH_TEST);

    // Base mesh
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

        if (m_BaseMeshWireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        m_BaseMesh.Draw();
        if (m_BaseMeshWireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Dual graph
    if (m_ShowDual && m_WireShader.IsValid())
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

// ============================================================
// ImGui
// ============================================================

void MeshDualDemo::OnImGui()
{
    if (ImGui::SliderInt("Subdivisions", &m_Subdivisions, 1, 4))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Sphere Radius", &m_SphereRadius, 1.0f, 6.0f))
        m_NeedRebuild = true;

    ImGui::Separator();
    ImGui::Text("Dual Graph");
    ImGui::SliderFloat("Strut Radius", &m_StrutRadius, 0.01f, 0.15f);
    ImGui::SliderFloat("Joint Radius", &m_JointRadius, 0.02f, 0.2f);
    ImGui::Checkbox("Show Dual", &m_ShowDual);

    ImGui::Separator();
    ImGui::Text("Base Mesh");
    ImGui::Checkbox("Show Base Mesh", &m_ShowBaseMesh);
    ImGui::Checkbox("Wireframe", &m_BaseMeshWireframe);

    ImGui::Separator();
    ImGui::Text("Colors");
    if (ImGui::ColorEdit3("Strut Color", m_StrutColor)) m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Joint Color", m_JointColor)) m_NeedRebuild = true;
    ImGui::ColorEdit3("Mesh Color", m_MeshColor);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate) ImGui::SliderFloat("Speed", &m_RotSpeed, 0.0f, 3.0f);

    ImGui::Separator();
    ImGui::Text("Stats: %d dual nodes, %d dual edges", m_DualNodeCount, m_DualEdgeCount);
}

// ============================================================
// 对偶图构建
// ============================================================

void MeshDualDemo::Regenerate()
{
    m_JointInst.clear();
    m_StrutInst.clear();

    glm::vec3 sCol(m_StrutColor[0], m_StrutColor[1], m_StrutColor[2]);
    glm::vec3 jCol(m_JointColor[0], m_JointColor[1], m_JointColor[2]);

    // 1. 生成 icosphere
    auto sphere = IcoSphere::Generate(m_Subdivisions, m_SphereRadius);
    int numTris = static_cast<int>(sphere.indices.size()) / 3;

    // Upload base mesh for rendering
    {
        std::vector<float> meshData;
        for (auto &v : sphere.vertices)
        {
            glm::vec3 n = glm::normalize(v);
            meshData.insert(meshData.end(), {v.x, v.y, v.z, n.x, n.y, n.z});
        }
        m_BaseMesh.Upload(meshData, sphere.indices);
    }

    // 2. 计算每个三角形的重心 (= 对偶图的节点)
    std::vector<glm::vec3> barycenters(numTris);
    for (int i = 0; i < numTris; i++)
    {
        glm::vec3 a = sphere.vertices[sphere.indices[3 * i]];
        glm::vec3 b = sphere.vertices[sphere.indices[3 * i + 1]];
        glm::vec3 c = sphere.vertices[sphere.indices[3 * i + 2]];
        barycenters[i] = (a + b + c) / 3.0f;
    }

    // 3. 构建 edge → triangle 邻接表
    //    key = (min_vertex, max_vertex), value = list of triangle indices
    std::unordered_map<uint64_t, std::vector<int>> edgeToTri;

    auto edgeKey = [](uint32_t a, uint32_t b) -> uint64_t
    {
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    };

    for (int i = 0; i < numTris; i++)
    {
        uint32_t v0 = sphere.indices[3 * i];
        uint32_t v1 = sphere.indices[3 * i + 1];
        uint32_t v2 = sphere.indices[3 * i + 2];
        edgeToTri[edgeKey(v0, v1)].push_back(i);
        edgeToTri[edgeKey(v1, v2)].push_back(i);
        edgeToTri[edgeKey(v2, v0)].push_back(i);
    }

    // 4. 对偶边: 连接共享边的两个三角形重心
    for (auto &[key, tris] : edgeToTri)
    {
        if (tris.size() == 2)
        {
            m_StrutInst.push_back({barycenters[tris[0]], barycenters[tris[1]], sCol});
        }
    }

    // 对偶节点 (投影到球面, 使节点在球面上)
    for (int i = 0; i < numTris; i++)
    {
        glm::vec3 projected = glm::normalize(barycenters[i]) * m_SphereRadius;
        m_JointInst.push_back({projected, jCol, glm::vec3(0)});
    }

    // 对偶边也投影到球面
    for (auto &s : m_StrutInst)
    {
        s.a = glm::normalize(s.a) * m_SphereRadius;
        s.b = glm::normalize(s.b) * m_SphereRadius;
    }

    m_DualNodeCount = numTris;
    m_DualEdgeCount = static_cast<int>(m_StrutInst.size());

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

void MeshDualDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_WireShader);
    hotReload.Watch(&m_SurfShader);
}
