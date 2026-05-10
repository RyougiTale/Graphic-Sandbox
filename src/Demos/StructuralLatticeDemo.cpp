#include "Demos/StructuralLatticeDemo.h"
#include "Camera/ICamera.h"
#include "Geometry/MarchingCubes.h"
#include "Geometry/Primitives.h"
#include "Graphics/ShaderHotReload.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>

static float smoothstep01(float x)
{
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

static float sdBox(const glm::vec3 &p, const glm::vec3 &b)
{
    glm::vec3 q = glm::abs(p) - b;
    glm::vec3 outside(std::max(q.x, 0.0f), std::max(q.y, 0.0f), std::max(q.z, 0.0f));
    float inside = std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
    return glm::length(outside) + inside;
}

static void setupMeshAttribs(GLuint vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

static void setupInstanceAttribs(GLuint instVBO, GLsizei stride,
                                 size_t offA, size_t offB, size_t offC, size_t offRadius)
{
    glBindBuffer(GL_ARRAY_BUFFER, instVBO);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(offA));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(offB));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(offC));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(offRadius));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
}

static std::array<int, 3> quantizePoint(const glm::vec3 &p)
{
    constexpr float q = 1000.0f;
    return {static_cast<int>(std::round(p.x * q)),
            static_cast<int>(std::round(p.y * q)),
            static_cast<int>(std::round(p.z * q))};
}

static std::array<int, 6> makeEdgeKey(const glm::vec3 &a, const glm::vec3 &b)
{
    auto qa = quantizePoint(a);
    auto qb = quantizePoint(b);
    if (qb < qa)
        std::swap(qa, qb);
    return {qa[0], qa[1], qa[2], qb[0], qb[1], qb[2]};
}

static bool isStrutMode(StructuralBuildMode mode)
{
    return mode == StructuralBuildMode::GraphStruts ||
           mode == StructuralBuildMode::VoronoiStruts;
}

static bool isImplicitMode(StructuralBuildMode mode)
{
    return mode == StructuralBuildMode::TPMSSurface ||
           mode == StructuralBuildMode::VoronoiFoam;
}

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

void StructuralLatticeDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/structural_lattice/structural.vert",
                           "shaders/Demos/structural_lattice/structural.frag");
    m_SurfaceShader.LoadFromFiles("shaders/Demos/voronoi_sponge/sponge.vert",
                                  "shaders/Demos/voronoi_sponge/sponge.frag");

    {
        std::vector<float> verts;
        std::vector<unsigned int> indices;
        Primitives::GenerateSphere(verts, indices, 1.0f, 16, 12);
        m_SphereIndexCount = static_cast<int>(indices.size());

        glGenBuffers(1, &m_SphereVBO);
        glGenBuffers(1, &m_SphereEBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_SphereVBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                     indices.data(), GL_STATIC_DRAW);
    }

    {
        std::vector<float> verts;
        std::vector<unsigned int> indices;
        Primitives::GenerateCylinder(verts, indices, 1.0f, 10);
        m_CylIndexCount = static_cast<int>(indices.size());

        glGenBuffers(1, &m_CylVBO);
        glGenBuffers(1, &m_CylEBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_CylVBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                     indices.data(), GL_STATIC_DRAW);
    }

    const GLsizei stride = static_cast<GLsizei>(sizeof(InstanceData));
    const size_t offA = offsetof(InstanceData, a);
    const size_t offB = offsetof(InstanceData, b);
    const size_t offC = offsetof(InstanceData, c);
    const size_t offRadius = offsetof(InstanceData, radius);

    glGenVertexArrays(1, &m_StrutVAO);
    glGenBuffers(1, &m_StrutInstVBO);
    glBindVertexArray(m_StrutVAO);
    setupMeshAttribs(m_CylVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CylEBO);
    setupInstanceAttribs(m_StrutInstVBO, stride, offA, offB, offC, offRadius);
    glBindVertexArray(0);

    glGenVertexArrays(1, &m_JointVAO);
    glGenBuffers(1, &m_JointInstVBO);
    glBindVertexArray(m_JointVAO);
    setupMeshAttribs(m_SphereVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SphereEBO);
    setupInstanceAttribs(m_JointInstVBO, stride, offA, offB, offC, offRadius);
    glBindVertexArray(0);

    glGenVertexArrays(1, &m_DomainVAO);
    glGenBuffers(1, &m_DomainVBO);

    m_NeedRebuild = true;
}

void StructuralLatticeDemo::OnDestroy()
{
    auto delBuf = [](GLuint &b)
    { if (b) { glDeleteBuffers(1, &b); b = 0; } };
    auto delVAO = [](GLuint &v)
    { if (v) { glDeleteVertexArrays(1, &v); v = 0; } };

    delVAO(m_JointVAO);
    delVAO(m_StrutVAO);
    delVAO(m_DomainVAO);
    delBuf(m_JointInstVBO);
    delBuf(m_StrutInstVBO);
    delBuf(m_DomainVBO);
    delBuf(m_SphereVBO);
    delBuf(m_SphereEBO);
    delBuf(m_CylVBO);
    delBuf(m_CylEBO);
    m_ImplicitMesh.Destroy();
}

void StructuralLatticeDemo::OnUpdate(float deltaTime)
{
    if (m_AutoRotate)
        m_RotAngle += m_RotSpeed * deltaTime;

    if (m_NeedRebuild)
    {
        RebuildLattice();
        m_NeedRebuild = false;
    }
}

void StructuralLatticeDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_RotAngle, glm::vec3(0.0f, 1.0f, 0.0f));

    glEnable(GL_DEPTH_TEST);

    if (isStrutMode(m_BuildMode))
    {
        if (!m_Shader.IsValid())
            return;

        m_Shader.Use();
        m_Shader.SetMat4("u_Model", model);
        m_Shader.SetMat4("u_View", camera.GetViewMatrix());
        m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
        m_Shader.SetVec3("u_LightDir", glm::normalize(glm::vec3(0.45f, 1.0f, 0.75f)));
        m_Shader.SetVec3("u_ViewPos", camera.GetPosition());

        if (m_ShowStruts && m_StrutCount > 0)
        {
            m_Shader.SetInt("u_RenderMode", 1);
            glBindVertexArray(m_StrutVAO);
            glDrawElementsInstanced(GL_TRIANGLES, m_CylIndexCount, GL_UNSIGNED_INT, nullptr, m_StrutCount);
        }

        if (m_ShowJoints && m_JointCount > 0)
        {
            m_Shader.SetInt("u_RenderMode", 0);
            glBindVertexArray(m_JointVAO);
            glDrawElementsInstanced(GL_TRIANGLES, m_SphereIndexCount, GL_UNSIGNED_INT, nullptr, m_JointCount);
        }
    }
    else if (isImplicitMode(m_BuildMode) && m_SurfaceShader.IsValid() && m_ImplicitMesh.IsValid())
    {
        m_SurfaceShader.Use();
        m_SurfaceShader.SetMat4("u_Model", model);
        m_SurfaceShader.SetMat4("u_View", camera.GetViewMatrix());
        m_SurfaceShader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
        m_SurfaceShader.SetVec3("u_LightDir", glm::normalize(glm::vec3(0.45f, 1.0f, 0.75f)));
        m_SurfaceShader.SetVec3("u_ViewPos", camera.GetPosition());
        m_SurfaceShader.SetVec3("u_Color", glm::vec3(m_StrutColor[0], m_StrutColor[1], m_StrutColor[2]));
        m_SurfaceShader.SetVec3("u_BackColor", glm::vec3(m_JointColor[0], m_JointColor[1], m_JointColor[2]));
        m_ImplicitMesh.Draw();
    }

    if (m_ShowDomain && m_DomainVertCount > 0 && m_Shader.IsValid())
    {
        m_Shader.Use();
        m_Shader.SetMat4("u_Model", model);
        m_Shader.SetMat4("u_View", camera.GetViewMatrix());
        m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
        m_Shader.SetInt("u_RenderMode", 2);
        glBindVertexArray(m_DomainVAO);
        glDrawArrays(GL_LINES, 0, m_DomainVertCount);
    }

    glBindVertexArray(0);
}

void StructuralLatticeDemo::OnImGui()
{
    const char *modeNames[] = {"Graph Struts", "Voronoi Struts", "TPMS Surface", "Voronoi Foam"};
    int modeIdx = static_cast<int>(m_BuildMode);
    if (ImGui::Combo("Structure Mode", &modeIdx, modeNames, IM_ARRAYSIZE(modeNames)))
    {
        m_BuildMode = static_cast<StructuralBuildMode>(modeIdx);
        m_NeedRebuild = true;
    }

    const char *cellNames[] = {"Cubic", "BCC", "Octet", "Diamond-like"};
    int cellIdx = static_cast<int>(m_CellType);
    if (m_BuildMode == StructuralBuildMode::GraphStruts &&
        ImGui::Combo("Graph Unit Cell", &cellIdx, cellNames, IM_ARRAYSIZE(cellNames)))
    {
        m_CellType = static_cast<StructuralCellType>(cellIdx);
        m_NeedRebuild = true;
    }

    if (m_BuildMode == StructuralBuildMode::TPMSSurface)
    {
        const char *tpmsNames[] = {"Gyroid", "Diamond TPMS"};
        int tpmsIdx = static_cast<int>(m_TPMSType);
        if (ImGui::Combo("TPMS Unit Cell", &tpmsIdx, tpmsNames, IM_ARRAYSIZE(tpmsNames)))
        {
            m_TPMSType = static_cast<StructuralTPMSType>(tpmsIdx);
            m_NeedRebuild = true;
        }
        if (ImGui::SliderFloat("TPMS Thickness", &m_TPMSThickness, 0.02f, 0.45f))
            m_NeedRebuild = true;
        if (ImGui::SliderFloat("TPMS Iso", &m_TPMSIsoValue, -0.8f, 0.8f))
            m_NeedRebuild = true;
    }

    if (m_BuildMode == StructuralBuildMode::VoronoiStruts ||
        m_BuildMode == StructuralBuildMode::VoronoiFoam)
    {
        if (m_BuildMode == StructuralBuildMode::VoronoiFoam)
        {
            if (ImGui::SliderFloat("Voronoi Foam Thickness", &m_VoronoiThickness, 0.02f, 0.35f))
                m_NeedRebuild = true;
        }
        if (ImGui::SliderFloat("Voronoi Jitter", &m_VoronoiJitter, 0.0f, 1.0f))
            m_NeedRebuild = true;
        if (ImGui::SliderInt("Voronoi Seed", &m_VoronoiSeed, 0, 255))
            m_NeedRebuild = true;
        if (m_BuildMode == StructuralBuildMode::VoronoiStruts &&
            ImGui::SliderInt("Nearest Neighbors", &m_VoronoiNeighborCount, 2, 8))
            m_NeedRebuild = true;
    }

    const char *domainNames[] = {"Box SDF", "Sphere SDF", "Shoe Sole SDF"};
    int domainIdx = static_cast<int>(m_DomainType);
    if (ImGui::Combo("Domain", &domainIdx, domainNames, IM_ARRAYSIZE(domainNames)))
    {
        m_DomainType = static_cast<StructuralDomainType>(domainIdx);
        m_NeedRebuild = true;
    }

    if (isStrutMode(m_BuildMode))
    {
        const char *fieldNames[] = {"Uniform", "Boundary Dense", "Vertical Gradient", "Center Dense"};
        int fieldIdx = static_cast<int>(m_FieldType);
        if (ImGui::Combo("Thickness Field", &fieldIdx, fieldNames, IM_ARRAYSIZE(fieldNames)))
        {
            m_FieldType = static_cast<StructuralFieldType>(fieldIdx);
            m_NeedRebuild = true;
        }
    }

    if (ImGui::SliderFloat3("Domain Size", &m_DomainSize.x, 0.8f, 9.0f))
        m_NeedRebuild = true;
    if (ImGui::SliderFloat("Cell Size", &m_CellSize, 0.35f, 1.4f))
        m_NeedRebuild = true;
    if (isStrutMode(m_BuildMode))
    {
        if (ImGui::SliderFloat("Strut Radius", &m_StrutRadius, 0.01f, 0.12f))
            m_NeedRebuild = true;
        if (ImGui::SliderFloat("Joint Scale", &m_JointScale, 0.8f, 2.0f))
            m_NeedRebuild = true;
        if (ImGui::SliderFloat("Field Strength", &m_FieldStrength, 0.0f, 2.0f))
            m_NeedRebuild = true;
        if (ImGui::SliderFloat("Boundary Band", &m_FieldBand, 0.1f, 2.0f))
            m_NeedRebuild = true;
        if (ImGui::SliderInt("Max Struts", &m_MaxStruts, 1000, 30000))
            m_NeedRebuild = true;
    }
    else
    {
        if (ImGui::SliderInt("MC Resolution", &m_ImplicitResolution, 48, 160))
            m_NeedRebuild = true;
    }

    ImGui::Separator();
    if (ImGui::ColorEdit3("Strut Color", m_StrutColor))
        m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Joint Color", m_JointColor))
        m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Domain Color", m_DomainColor))
        m_NeedRebuild = true;

    ImGui::Separator();
    if (isStrutMode(m_BuildMode))
    {
        ImGui::Checkbox("Show Struts", &m_ShowStruts);
        ImGui::Checkbox("Show Joints", &m_ShowJoints);
    }
    ImGui::Checkbox("Show Domain", &m_ShowDomain);
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Rotation Speed", &m_RotSpeed, 0.0f, 3.0f);

    ImGui::Separator();
    if (isStrutMode(m_BuildMode))
    {
        ImGui::Text("Cells: %d  Unit Edges: %d", m_NumCells, m_NumUnitEdges);
        ImGui::Text("Struts: %d  Joints: %d%s",
                    m_StrutCount, m_JointCount, m_Truncated ? "  (truncated)" : "");
    }
    else
    {
        ImGui::Text("Implicit: %d tris, %d verts", m_ImplicitTriCount, m_ImplicitVertCount);
    }
}

std::vector<StructuralLatticeDemo::CellEdge> StructuralLatticeDemo::BuildUnitCell() const
{
    std::vector<CellEdge> edges;
    auto add = [&](glm::vec3 a, glm::vec3 b)
    {
        edges.push_back({a, b});
    };

    std::array<glm::vec3, 8> c = {
        glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(1, 1, 0),
        glm::vec3(0, 0, 1), glm::vec3(1, 0, 1), glm::vec3(0, 1, 1), glm::vec3(1, 1, 1)
    };

    switch (m_CellType)
    {
    case StructuralCellType::Cubic:
        for (int i = 0; i < 8; ++i)
        {
            for (int j = i + 1; j < 8; ++j)
            {
                glm::vec3 d = glm::abs(c[i] - c[j]);
                if (std::abs(d.x + d.y + d.z - 1.0f) < 0.001f)
                    add(c[i], c[j]);
            }
        }
        break;

    case StructuralCellType::BodyCentered:
    {
        glm::vec3 center(0.5f);
        for (const auto &corner : c)
            add(center, corner);
        break;
    }

    case StructuralCellType::Octet:
    {
        std::array<glm::vec3, 6> f = {
            glm::vec3(0.0f, 0.5f, 0.5f), glm::vec3(1.0f, 0.5f, 0.5f),
            glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(0.5f, 1.0f, 0.5f),
            glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.5f, 0.5f, 1.0f)
        };
        for (const auto &fc : f)
        {
            for (const auto &corner : c)
            {
                bool onFace = (std::abs(fc.x - 0.0f) < 0.001f && std::abs(corner.x - 0.0f) < 0.001f) ||
                              (std::abs(fc.x - 1.0f) < 0.001f && std::abs(corner.x - 1.0f) < 0.001f) ||
                              (std::abs(fc.y - 0.0f) < 0.001f && std::abs(corner.y - 0.0f) < 0.001f) ||
                              (std::abs(fc.y - 1.0f) < 0.001f && std::abs(corner.y - 1.0f) < 0.001f) ||
                              (std::abs(fc.z - 0.0f) < 0.001f && std::abs(corner.z - 0.0f) < 0.001f) ||
                              (std::abs(fc.z - 1.0f) < 0.001f && std::abs(corner.z - 1.0f) < 0.001f);
                if (onFace)
                    add(fc, corner);
            }
        }
        break;
    }

    case StructuralCellType::Diamond:
    {
        glm::vec3 center(0.5f);
        add(center, glm::vec3(0, 0, 0));
        add(center, glm::vec3(1, 1, 0));
        add(center, glm::vec3(1, 0, 1));
        add(center, glm::vec3(0, 1, 1));
        break;
    }
    }

    return edges;
}

float StructuralLatticeDemo::SoleHalfWidthAt(float x) const
{
    float hx = std::max(m_DomainSize.x * 0.5f, 0.001f);
    float u = std::clamp((x + hx) / (2.0f * hx), 0.0f, 1.0f);
    float heelToForefoot = 0.68f + 0.32f * smoothstep01((u - 0.18f) / 0.52f);
    float toeTaper = 1.0f - 0.18f * smoothstep01((u - 0.82f) / 0.18f);
    return m_DomainSize.z * 0.5f * heelToForefoot * toeTaper;
}

float StructuralLatticeDemo::DomainSDF(const glm::vec3 &p) const
{
    glm::vec3 half = m_DomainSize * 0.5f;
    switch (m_DomainType)
    {
    case StructuralDomainType::Box:
        return sdBox(p, half);
    case StructuralDomainType::Sphere:
        return glm::length(p) - std::min(half.x, std::min(half.y, half.z));
    case StructuralDomainType::ShoeSole:
    {
        float hx = half.x;
        float hy = half.y;
        float hz = SoleHalfWidthAt(p.x);
        glm::vec3 q(std::abs(p.x) - hx, std::abs(p.y) - hy, std::abs(p.z) - hz);
        glm::vec3 outside(std::max(q.x, 0.0f), std::max(q.y, 0.0f), std::max(q.z, 0.0f));
        float inside = std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
        return glm::length(outside) + inside;
    }
    }
    return 1.0f;
}

float StructuralLatticeDemo::RadiusAt(const glm::vec3 &p) const
{
    float factor = 1.0f;
    switch (m_FieldType)
    {
    case StructuralFieldType::Uniform:
        factor = 1.0f;
        break;
    case StructuralFieldType::BoundaryDense:
    {
        float insideDistance = std::max(-DomainSDF(p), 0.0f);
        float t = 1.0f - std::clamp(insideDistance / std::max(m_FieldBand, 0.001f), 0.0f, 1.0f);
        factor = 1.0f + m_FieldStrength * t;
        break;
    }
    case StructuralFieldType::VerticalGradient:
    {
        float t = std::clamp(p.y / std::max(m_DomainSize.y, 0.001f) + 0.5f, 0.0f, 1.0f);
        factor = 0.7f + m_FieldStrength * t;
        break;
    }
    case StructuralFieldType::CenterDense:
    {
        float nx = p.x / std::max(m_DomainSize.x * 0.5f, 0.001f);
        float nz = p.z / std::max(m_DomainSize.z * 0.5f, 0.001f);
        float t = 1.0f - std::clamp(std::sqrt(nx * nx + nz * nz), 0.0f, 1.0f);
        factor = 1.0f + m_FieldStrength * t;
        break;
    }
    }
    return std::max(0.004f, m_StrutRadius * factor);
}

bool StructuralLatticeDemo::ClipSegmentToDomain(const glm::vec3 &p0, const glm::vec3 &p1,
                                                glm::vec3 &outA, glm::vec3 &outB) const
{
    constexpr int samples = 8;
    float prevT = 0.0f;
    float prevD = DomainSDF(p0);
    bool prevInside = prevD <= 0.0f;
    bool active = prevInside;
    glm::vec3 start = p0;

    for (int i = 1; i <= samples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(samples);
        glm::vec3 p = p0 + (p1 - p0) * t;
        float d = DomainSDF(p);
        bool inside = d <= 0.0f;

        if (inside != prevInside)
        {
            float denom = std::abs(prevD) + std::abs(d);
            float local = denom > 1e-6f ? std::abs(prevD) / denom : 0.5f;
            float crossT = prevT + (t - prevT) * local;
            glm::vec3 crossP = p0 + (p1 - p0) * crossT;

            if (!prevInside && inside)
            {
                start = crossP;
                active = true;
            }
            else if (prevInside && !inside && active)
            {
                outA = start;
                outB = crossP;
                return glm::length(outB - outA) > 0.02f;
            }
        }

        prevT = t;
        prevD = d;
        prevInside = inside;
    }

    if (active && prevInside)
    {
        outA = start;
        outB = p1;
        return glm::length(outB - outA) > 0.02f;
    }

    return false;
}

void StructuralLatticeDemo::RebuildLattice()
{
    if (m_BuildMode == StructuralBuildMode::GraphStruts)
    {
        m_ImplicitMesh.Destroy();
        m_ImplicitTriCount = 0;
        m_ImplicitVertCount = 0;
        RebuildGraphLattice();
    }
    else if (m_BuildMode == StructuralBuildMode::VoronoiStruts)
    {
        m_ImplicitMesh.Destroy();
        m_ImplicitTriCount = 0;
        m_ImplicitVertCount = 0;
        RebuildVoronoiStruts();
    }
    else
    {
        m_JointInstances.clear();
        m_StrutInstances.clear();
        m_JointCount = 0;
        m_StrutCount = 0;
        RebuildImplicitLattice();
    }
}

void StructuralLatticeDemo::RebuildGraphLattice()
{
    m_JointInstances.clear();
    m_StrutInstances.clear();
    m_Truncated = false;

    glm::vec3 strutCol(m_StrutColor[0], m_StrutColor[1], m_StrutColor[2]);
    glm::vec3 jointCol(m_JointColor[0], m_JointColor[1], m_JointColor[2]);
    std::set<std::array<int, 6>> edgeKeys;
    std::set<std::array<int, 3>> jointKeys;

    auto addJoint = [&](const glm::vec3 &p)
    {
        auto key = quantizePoint(p);
        if (!jointKeys.insert(key).second)
            return;
        m_JointInstances.push_back({p, glm::vec3(0.0f), jointCol, RadiusAt(p) * m_JointScale});
    };

    auto addStrut = [&](const glm::vec3 &a, const glm::vec3 &b)
    {
        if (m_StrutInstances.size() >= static_cast<size_t>(m_MaxStruts))
        {
            m_Truncated = true;
            return;
        }

        auto key = makeEdgeKey(a, b);
        if (!edgeKeys.insert(key).second)
            return;

        glm::vec3 mid = (a + b) * 0.5f;
        m_StrutInstances.push_back({a, b, strutCol, RadiusAt(mid)});
        addJoint(a);
        addJoint(b);
    };

    std::vector<CellEdge> unitEdges = BuildUnitCell();
    m_NumUnitEdges = static_cast<int>(unitEdges.size());

    float cell = std::max(m_CellSize, 0.05f);
    glm::ivec3 dims(
        std::max(1, static_cast<int>(std::ceil(m_DomainSize.x / cell))),
        std::max(1, static_cast<int>(std::ceil(m_DomainSize.y / cell))),
        std::max(1, static_cast<int>(std::ceil(m_DomainSize.z / cell))));
    m_NumCells = dims.x * dims.y * dims.z;

    glm::vec3 gridSize = glm::vec3(dims) * cell;
    glm::vec3 origin = -gridSize * 0.5f;

    for (int ix = 0; ix < dims.x && !m_Truncated; ++ix)
    for (int iy = 0; iy < dims.y && !m_Truncated; ++iy)
    for (int iz = 0; iz < dims.z && !m_Truncated; ++iz)
    {
        glm::vec3 base = origin + glm::vec3(ix, iy, iz) * cell;
        for (const auto &e : unitEdges)
        {
            glm::vec3 p0 = base + e.a * cell;
            glm::vec3 p1 = base + e.b * cell;
            glm::vec3 clippedA, clippedB;
            if (ClipSegmentToDomain(p0, p1, clippedA, clippedB))
                addStrut(clippedA, clippedB);
            if (m_Truncated)
                break;
        }
    }

    UploadInstances();
    UploadDomainWireframe();
}

void StructuralLatticeDemo::RebuildVoronoiStruts()
{
    m_JointInstances.clear();
    m_StrutInstances.clear();
    m_Truncated = false;
    m_NumUnitEdges = 0;

    glm::vec3 strutCol(m_StrutColor[0], m_StrutColor[1], m_StrutColor[2]);
    glm::vec3 jointCol(m_JointColor[0], m_JointColor[1], m_JointColor[2]);
    std::set<std::array<int, 6>> edgeKeys;
    std::set<std::array<int, 3>> jointKeys;

    auto addJoint = [&](const glm::vec3 &p)
    {
        auto key = quantizePoint(p);
        if (!jointKeys.insert(key).second)
            return;
        m_JointInstances.push_back({p, glm::vec3(0.0f), jointCol, RadiusAt(p) * m_JointScale});
    };

    auto addStrut = [&](const glm::vec3 &a, const glm::vec3 &b)
    {
        if (m_StrutInstances.size() >= static_cast<size_t>(m_MaxStruts))
        {
            m_Truncated = true;
            return;
        }

        auto key = makeEdgeKey(a, b);
        if (!edgeKeys.insert(key).second)
            return;

        glm::vec3 mid = (a + b) * 0.5f;
        m_StrutInstances.push_back({a, b, strutCol, RadiusAt(mid)});
        addJoint(a);
        addJoint(b);
    };

    float cell = std::max(m_CellSize, 0.05f);
    glm::ivec3 dims(
        std::max(1, static_cast<int>(std::ceil(m_DomainSize.x / cell))),
        std::max(1, static_cast<int>(std::ceil(m_DomainSize.y / cell))),
        std::max(1, static_cast<int>(std::ceil(m_DomainSize.z / cell))));
    m_NumCells = dims.x * dims.y * dims.z;

    glm::vec3 gridSize = glm::vec3(dims) * cell;
    glm::vec3 origin = -gridSize * 0.5f;
    std::vector<glm::vec3> seeds;
    seeds.reserve(static_cast<size_t>(m_NumCells));

    for (int ix = 0; ix < dims.x; ++ix)
    for (int iy = 0; iy < dims.y; ++iy)
    for (int iz = 0; iz < dims.z; ++iz)
    {
        glm::vec3 jitter(
            hashFloat(ix, iy, iz, m_VoronoiSeed, 0) - 0.5f,
            hashFloat(ix, iy, iz, m_VoronoiSeed, 1) - 0.5f,
            hashFloat(ix, iy, iz, m_VoronoiSeed, 2) - 0.5f);
        glm::vec3 p = origin + (glm::vec3(ix, iy, iz) + glm::vec3(0.5f)) * cell
                    + jitter * (m_VoronoiJitter * cell * 0.65f);
        if (DomainSDF(p) <= 0.0f)
            seeds.push_back(p);
    }

    float maxDist = cell * 1.9f;
    for (size_t i = 0; i < seeds.size() && !m_Truncated; ++i)
    {
        std::vector<std::pair<float, size_t>> neighbors;
        neighbors.reserve(seeds.size());
        for (size_t j = 0; j < seeds.size(); ++j)
        {
            if (i == j)
                continue;
            float d = glm::length(seeds[j] - seeds[i]);
            if (d <= maxDist)
                neighbors.push_back({d, j});
        }

        std::sort(neighbors.begin(), neighbors.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });

        int n = std::min(m_VoronoiNeighborCount, static_cast<int>(neighbors.size()));
        for (int k = 0; k < n; ++k)
        {
            glm::vec3 clippedA, clippedB;
            if (ClipSegmentToDomain(seeds[i], seeds[neighbors[k].second], clippedA, clippedB))
                addStrut(clippedA, clippedB);
            if (m_Truncated)
                break;
        }
    }

    m_NumUnitEdges = m_VoronoiNeighborCount;
    UploadInstances();
    UploadDomainWireframe();
}

float StructuralLatticeDemo::TPMSField(const glm::vec3 &p) const
{
    float k = 6.2831853f / std::max(m_CellSize, 0.001f);
    float x = p.x * k;
    float y = p.y * k;
    float z = p.z * k;

    if (m_TPMSType == StructuralTPMSType::Gyroid)
        return std::sin(x) * std::cos(y) + std::sin(y) * std::cos(z) + std::sin(z) * std::cos(x);

    return std::sin(x) * std::sin(y) * std::sin(z)
         + std::sin(x) * std::cos(y) * std::cos(z)
         + std::cos(x) * std::sin(y) * std::cos(z)
         + std::cos(x) * std::cos(y) * std::sin(z);
}

float StructuralLatticeDemo::TPMSLatticeSDF(const glm::vec3 &p) const
{
    return std::abs(TPMSField(p) - m_TPMSIsoValue) - m_TPMSThickness;
}

float StructuralLatticeDemo::VoronoiLatticeSDF(const glm::vec3 &p) const
{
    float gx = p.x / std::max(m_CellSize, 0.001f);
    float gy = p.y / std::max(m_CellSize, 0.001f);
    float gz = p.z / std::max(m_CellSize, 0.001f);
    int cx = static_cast<int>(std::floor(gx));
    int cy = static_cast<int>(std::floor(gy));
    int cz = static_cast<int>(std::floor(gz));

    float d1 = 1e10f;
    float d2 = 1e10f;
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx)
    {
        int nx = cx + dx;
        int ny = cy + dy;
        int nz = cz + dz;
        glm::vec3 seed(
            static_cast<float>(nx) + 0.5f + (hashFloat(nx, ny, nz, m_VoronoiSeed, 0) - 0.5f) * m_VoronoiJitter,
            static_cast<float>(ny) + 0.5f + (hashFloat(nx, ny, nz, m_VoronoiSeed, 1) - 0.5f) * m_VoronoiJitter,
            static_cast<float>(nz) + 0.5f + (hashFloat(nx, ny, nz, m_VoronoiSeed, 2) - 0.5f) * m_VoronoiJitter);
        glm::vec3 d = glm::vec3(gx, gy, gz) - seed;
        float dist = glm::length(d) * m_CellSize;
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

    return (d2 - d1) - m_VoronoiThickness;
}

void StructuralLatticeDemo::RebuildImplicitLattice()
{
    float margin = std::max({m_TPMSThickness, m_VoronoiThickness, 0.05f}) + 0.1f;
    glm::vec3 half = m_DomainSize * 0.5f + glm::vec3(margin);

    auto sdf = [&](float x, float y, float z) -> float
    {
        glm::vec3 p(x, y, z);
        float lattice = (m_BuildMode == StructuralBuildMode::TPMSSurface)
                            ? TPMSLatticeSDF(p)
                            : VoronoiLatticeSDF(p);
        return std::max(DomainSDF(p), lattice);
    };

    auto result = MarchingCubes::Extract(sdf, -half, half, m_ImplicitResolution, 0.0f);
    m_ImplicitTriCount = static_cast<int>(result.indices.size() / 3);
    m_ImplicitVertCount = static_cast<int>(result.vertices.size());

    if (result.vertices.empty())
    {
        m_ImplicitMesh.Destroy();
    }
    else
    {
        m_ImplicitMesh.Upload(
            reinterpret_cast<const std::vector<MeshRenderer::MCVertex> &>(result.vertices),
            result.indices);
    }

    UploadDomainWireframe();
}

void StructuralLatticeDemo::UploadInstances()
{
    m_StrutCount = static_cast<int>(m_StrutInstances.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_StrutInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_StrutInstances.size() * sizeof(InstanceData)),
                 m_StrutInstances.data(), GL_DYNAMIC_DRAW);

    m_JointCount = static_cast<int>(m_JointInstances.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_JointInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_JointInstances.size() * sizeof(InstanceData)),
                 m_JointInstances.data(), GL_DYNAMIC_DRAW);
}

void StructuralLatticeDemo::UploadDomainWireframe()
{
    std::vector<float> verts;
    glm::vec3 color(m_DomainColor[0], m_DomainColor[1], m_DomainColor[2]);
    auto addLine = [&](glm::vec3 a, glm::vec3 b)
    {
        verts.insert(verts.end(), {a.x, a.y, a.z, color.r, color.g, color.b});
        verts.insert(verts.end(), {b.x, b.y, b.z, color.r, color.g, color.b});
    };

    glm::vec3 half = m_DomainSize * 0.5f;

    if (m_DomainType == StructuralDomainType::Sphere)
    {
        float r = std::min(half.x, std::min(half.y, half.z));
        constexpr int n = 96;
        for (int i = 0; i < n; ++i)
        {
            float a0 = 6.2831853f * static_cast<float>(i) / n;
            float a1 = 6.2831853f * static_cast<float>(i + 1) / n;
            addLine(glm::vec3(std::cos(a0) * r, std::sin(a0) * r, 0),
                    glm::vec3(std::cos(a1) * r, std::sin(a1) * r, 0));
            addLine(glm::vec3(std::cos(a0) * r, 0, std::sin(a0) * r),
                    glm::vec3(std::cos(a1) * r, 0, std::sin(a1) * r));
            addLine(glm::vec3(0, std::cos(a0) * r, std::sin(a0) * r),
                    glm::vec3(0, std::cos(a1) * r, std::sin(a1) * r));
        }
    }
    else if (m_DomainType == StructuralDomainType::ShoeSole)
    {
        constexpr int n = 48;
        for (int i = 0; i < n; ++i)
        {
            float x0 = -half.x + m_DomainSize.x * static_cast<float>(i) / n;
            float x1 = -half.x + m_DomainSize.x * static_cast<float>(i + 1) / n;
            float z0 = SoleHalfWidthAt(x0);
            float z1 = SoleHalfWidthAt(x1);
            for (float y : {-half.y, half.y})
            {
                addLine(glm::vec3(x0, y, z0), glm::vec3(x1, y, z1));
                addLine(glm::vec3(x0, y, -z0), glm::vec3(x1, y, -z1));
            }
            if (i % 6 == 0)
            {
                addLine(glm::vec3(x0, -half.y, z0), glm::vec3(x0, half.y, z0));
                addLine(glm::vec3(x0, -half.y, -z0), glm::vec3(x0, half.y, -z0));
            }
        }
    }
    else
    {
        glm::vec3 bmin = -half;
        glm::vec3 dx(m_DomainSize.x, 0, 0), dy(0, m_DomainSize.y, 0), dz(0, 0, m_DomainSize.z);
        addLine(bmin, bmin + dx);           addLine(bmin, bmin + dy);           addLine(bmin, bmin + dz);
        addLine(bmin + dx, bmin + dx + dy); addLine(bmin + dx, bmin + dx + dz);
        addLine(bmin + dy, bmin + dy + dx); addLine(bmin + dy, bmin + dy + dz);
        addLine(bmin + dz, bmin + dz + dx); addLine(bmin + dz, bmin + dz + dy);
        addLine(bmin + dx + dy, half);      addLine(bmin + dx + dz, half);
        addLine(bmin + dy + dz, half);
    }

    m_DomainVertCount = static_cast<int>(verts.size() / 6);
    glBindVertexArray(m_DomainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_DomainVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void StructuralLatticeDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
    hotReload.Watch(&m_SurfaceShader);
}
