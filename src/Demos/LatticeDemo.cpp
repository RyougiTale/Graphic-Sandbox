#include "Demos/LatticeDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <cmath>

// ============================================================
// 初始化 / 销毁
// ============================================================

static void SetupInstanceAttributes(GLuint instanceVBO)
{
    // 3 个 vec3: loc2, loc3, loc4
    // 对应 InstanceData { vec3 a, vec3 b, vec3 c }
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    for (int i = 0; i < 3; i++)
    {
        glVertexAttribPointer(2 + i, 3, GL_FLOAT, GL_FALSE,
                              3 * sizeof(glm::vec3),
                              (void *)(i * sizeof(glm::vec3)));
        glEnableVertexAttribArray(2 + i);
        glVertexAttribDivisor(2 + i, 1); // 每实例
    }
}

void LatticeDemo::OnInit()
{
    m_Shader.LoadFromFiles("shaders/Demos/lattice/lattice.vert",
                           "shaders/Demos/lattice/lattice.frag");

    // --- 球体模板 (原子) ---
    {
        std::vector<float> verts;
        std::vector<unsigned int> indices;
        GenerateSphere(verts, indices, 1.0f, 16, 12);
        m_SphereIndexCount = static_cast<int>(indices.size());

        glGenVertexArrays(1, &m_AtomVAO);
        glGenBuffers(1, &m_AtomVBO);
        glGenBuffers(1, &m_AtomEBO);
        glGenBuffers(1, &m_AtomInstanceVBO);

        glBindVertexArray(m_AtomVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_AtomVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        // loc0 = position, loc1 = normal
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_AtomEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        SetupInstanceAttributes(m_AtomInstanceVBO);
        glBindVertexArray(0);
    }

    // --- 圆柱体模板 (键) ---
    {
        std::vector<float> verts;
        std::vector<unsigned int> indices;
        GenerateCylinder(verts, indices, 1.0f, 8);
        m_CylinderIndexCount = static_cast<int>(indices.size());

        glGenVertexArrays(1, &m_BondVAO);
        glGenBuffers(1, &m_BondVBO);
        glGenBuffers(1, &m_BondEBO);
        glGenBuffers(1, &m_BondInstanceVBO);

        glBindVertexArray(m_BondVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_BondVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BondEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        SetupInstanceAttributes(m_BondInstanceVBO);
        glBindVertexArray(0);
    }

    // --- 单元格线框 ---
    {
        glGenVertexArrays(1, &m_CellVAO);
        glGenBuffers(1, &m_CellVBO);
    }

    m_NeedRebuild = true;
}

void LatticeDemo::OnDestroy()
{
    auto cleanup = [](GLuint &vao, GLuint &vbo, GLuint &ebo, GLuint &inst)
    {
        if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
        if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
        if (inst) { glDeleteBuffers(1, &inst); inst = 0; }
    };
    cleanup(m_AtomVAO, m_AtomVBO, m_AtomEBO, m_AtomInstanceVBO);
    cleanup(m_BondVAO, m_BondVBO, m_BondEBO, m_BondInstanceVBO);
    if (m_CellVAO) { glDeleteVertexArrays(1, &m_CellVAO); m_CellVAO = 0; }
    if (m_CellVBO) { glDeleteBuffers(1, &m_CellVBO); m_CellVBO = 0; }
}

// ============================================================
// 更新 / 渲染
// ============================================================

void LatticeDemo::OnUpdate(float deltaTime)
{
    if (m_AutoRotate)
        m_RotationAngle += m_RotationSpeed * deltaTime;

    if (m_NeedRebuild)
    {
        RebuildLattice();
        m_NeedRebuild = false;
    }
}

void LatticeDemo::OnRender(const ICamera &camera, float aspectRatio)
{
    if (!m_Shader.IsValid())
        return;

    m_Shader.Use();

    // 居中晶格
    glm::mat4 model(1.0f);
    float cx = (m_RepeatX - 1) * m_LatticeConstant * 0.5f;
    float cy = (m_RepeatY - 1) * m_LatticeConstant * 0.5f;
    float cz = (m_RepeatZ - 1) * m_LatticeConstant * 0.5f;
    model = glm::rotate(model, m_RotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(-cx, -cy, -cz));

    m_Shader.SetMat4("u_Model", model);
    m_Shader.SetMat4("u_View", camera.GetViewMatrix());
    m_Shader.SetMat4("u_Projection", camera.GetProjectionMatrix(aspectRatio));
    m_Shader.SetVec3("u_LightDir", glm::normalize(glm::vec3(0.5f, 1.0f, 0.8f)));
    m_Shader.SetVec3("u_ViewPos", camera.GetPosition());

    glEnable(GL_DEPTH_TEST);

    // 画原子
    m_Shader.SetInt("u_RenderMode", 0);
    m_Shader.SetFloat("u_AtomRadius", m_AtomRadius);
    glBindVertexArray(m_AtomVAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_SphereIndexCount, GL_UNSIGNED_INT, nullptr, m_AtomCount);

    // 画键
    if (m_ShowBonds && m_BondCount > 0)
    {
        m_Shader.SetInt("u_RenderMode", 1);
        m_Shader.SetFloat("u_BondRadius", m_BondRadius);
        glBindVertexArray(m_BondVAO);
        glDrawElementsInstanced(GL_TRIANGLES, m_CylinderIndexCount, GL_UNSIGNED_INT, nullptr, m_BondCount);
    }

    // 画单元格线框
    if (m_ShowUnitCell && m_CellVertexCount > 0)
    {
        m_Shader.SetInt("u_RenderMode", 2);
        glBindVertexArray(m_CellVAO);
        glDrawArrays(GL_LINES, 0, m_CellVertexCount);
    }

    glBindVertexArray(0);
}

// ============================================================
// ImGui
// ============================================================

void LatticeDemo::OnImGui()
{
    const char *typeNames[] = {"Simple Cubic (SC)", "Body-Centered Cubic (BCC)",
                               "Face-Centered Cubic (FCC)", "Hexagonal (HCP)",
                               "Diamond"};
    int typeIdx = static_cast<int>(m_LatticeType);
    if (ImGui::Combo("Lattice Type", &typeIdx, typeNames, IM_ARRAYSIZE(typeNames)))
    {
        m_LatticeType = static_cast<LatticeType>(typeIdx);
        m_NeedRebuild = true;
    }

    if (ImGui::SliderInt("Repeat X", &m_RepeatX, 1, 8)) m_NeedRebuild = true;
    if (ImGui::SliderInt("Repeat Y", &m_RepeatY, 1, 8)) m_NeedRebuild = true;
    if (ImGui::SliderInt("Repeat Z", &m_RepeatZ, 1, 8)) m_NeedRebuild = true;
    if (ImGui::SliderFloat("Lattice Constant", &m_LatticeConstant, 0.5f, 5.0f)) m_NeedRebuild = true;
    ImGui::SliderFloat("Atom Radius", &m_AtomRadius, 0.05f, 1.0f);
    if (ImGui::SliderFloat("Bond Radius", &m_BondRadius, 0.01f, 0.2f)) m_NeedRebuild = true;
    if (ImGui::SliderFloat("Bond Cutoff", &m_BondCutoff, 0.5f, 2.0f, "%.2f x nearest")) m_NeedRebuild = true;

    if (ImGui::ColorEdit3("Atom Color 1", m_AtomColor1)) m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Atom Color 2", m_AtomColor2)) m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Bond Color", m_BondColor)) m_NeedRebuild = true;
    if (ImGui::ColorEdit3("Cell Color", m_CellColor)) m_NeedRebuild = true;

    ImGui::Checkbox("Show Bonds", &m_ShowBonds);
    ImGui::Checkbox("Show Unit Cell", &m_ShowUnitCell);
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 5.0f);

    ImGui::Separator();
    ImGui::Text("Atoms: %d  Bonds: %d", m_AtomCount, m_BondCount);
}

// ============================================================
// 晶格构建
// ============================================================

void LatticeDemo::RebuildLattice()
{
    m_AtomInstances.clear();
    m_BondInstances.clear();

    float a = m_LatticeConstant;
    glm::vec3 col1(m_AtomColor1[0], m_AtomColor1[1], m_AtomColor1[2]);
    glm::vec3 col2(m_AtomColor2[0], m_AtomColor2[1], m_AtomColor2[2]);
    glm::vec3 bondCol(m_BondColor[0], m_BondColor[1], m_BondColor[2]);

    // 收集所有原子位置 (用于后面生成键)
    struct AtomInfo { glm::vec3 pos; glm::vec3 color; };
    std::vector<AtomInfo> atoms;

    for (int ix = 0; ix < m_RepeatX; ix++)
    for (int iy = 0; iy < m_RepeatY; iy++)
    for (int iz = 0; iz < m_RepeatZ; iz++)
    {
        glm::vec3 o(ix * a, iy * a, iz * a);

        switch (m_LatticeType)
        {
        case LatticeType::SimpleCubic:
            atoms.push_back({o, col1});
            break;

        case LatticeType::BodyCentered:
            atoms.push_back({o, col1});
            atoms.push_back({o + glm::vec3(0.5f) * a, col2});
            break;

        case LatticeType::FaceCentered:
            atoms.push_back({o, col1});
            atoms.push_back({o + glm::vec3(0.5f, 0.5f, 0.0f) * a, col2});
            atoms.push_back({o + glm::vec3(0.5f, 0.0f, 0.5f) * a, col2});
            atoms.push_back({o + glm::vec3(0.0f, 0.5f, 0.5f) * a, col2});
            break;

        case LatticeType::Hexagonal:
        {
            float c = a * 1.633f;
            atoms.push_back({o, col1});
            atoms.push_back({o + glm::vec3(a * 0.5f, 0.0f, a * 0.289f), col1});
            atoms.push_back({o + glm::vec3(a * 0.25f, c * 0.5f, a * 0.144f), col2});
            break;
        }

        case LatticeType::Diamond:
        {
            atoms.push_back({o, col1});
            atoms.push_back({o + glm::vec3(0.5f, 0.5f, 0.0f) * a, col1});
            atoms.push_back({o + glm::vec3(0.5f, 0.0f, 0.5f) * a, col1});
            atoms.push_back({o + glm::vec3(0.0f, 0.5f, 0.5f) * a, col1});
            glm::vec3 off(0.25f * a);
            atoms.push_back({o + off, col2});
            atoms.push_back({o + off + glm::vec3(0.5f, 0.5f, 0.0f) * a, col2});
            atoms.push_back({o + off + glm::vec3(0.5f, 0.0f, 0.5f) * a, col2});
            atoms.push_back({o + off + glm::vec3(0.0f, 0.5f, 0.5f) * a, col2});
            break;
        }
        }
    }

    // 转为实例数据
    for (auto &at : atoms)
        m_AtomInstances.push_back({at.pos, at.color, glm::vec3(0)});

    // 计算最近邻距离
    float nn = a;
    switch (m_LatticeType)
    {
    case LatticeType::SimpleCubic:   nn = a; break;
    case LatticeType::BodyCentered:  nn = a * 0.866f; break;
    case LatticeType::FaceCentered:  nn = a * 0.7071f; break;
    case LatticeType::Hexagonal:     nn = a * 0.5f; break;
    case LatticeType::Diamond:       nn = a * 0.433f; break;
    }

    // 生成键
    float maxLen2 = nn * m_BondCutoff * nn * m_BondCutoff;
    for (size_t i = 0; i < atoms.size(); i++)
    {
        for (size_t j = i + 1; j < atoms.size(); j++)
        {
            glm::vec3 d = atoms[j].pos - atoms[i].pos;
            float dist2 = glm::dot(d, d);
            if (dist2 > 0.001f && dist2 < maxLen2)
                m_BondInstances.push_back({atoms[i].pos, atoms[j].pos, bondCol});
        }
    }

    UploadAtomMesh();
    UploadBondMesh();
    UploadCellMesh();
}

void LatticeDemo::UploadAtomMesh()
{
    m_AtomCount = static_cast<int>(m_AtomInstances.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_AtomInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_AtomInstances.size() * sizeof(InstanceData),
                 m_AtomInstances.data(), GL_DYNAMIC_DRAW);
}

void LatticeDemo::UploadBondMesh()
{
    m_BondCount = static_cast<int>(m_BondInstances.size());
    glBindBuffer(GL_ARRAY_BUFFER, m_BondInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_BondInstances.size() * sizeof(InstanceData),
                 m_BondInstances.data(), GL_DYNAMIC_DRAW);
}

void LatticeDemo::UploadCellMesh()
{
    std::vector<float> verts;
    float a = m_LatticeConstant;
    glm::vec3 cc(m_CellColor[0], m_CellColor[1], m_CellColor[2]);

    auto addLine = [&](glm::vec3 p0, glm::vec3 p1)
    {
        verts.insert(verts.end(), {p0.x, p0.y, p0.z, cc.r, cc.g, cc.b});
        verts.insert(verts.end(), {p1.x, p1.y, p1.z, cc.r, cc.g, cc.b});
    };

    for (int ix = 0; ix < m_RepeatX; ix++)
    for (int iy = 0; iy < m_RepeatY; iy++)
    for (int iz = 0; iz < m_RepeatZ; iz++)
    {
        glm::vec3 o(ix * a, iy * a, iz * a);
        glm::vec3 dx(a, 0, 0), dy(0, a, 0), dz(0, 0, a);
        addLine(o, o + dx);           addLine(o, o + dy);           addLine(o, o + dz);
        addLine(o + dx, o + dx + dy); addLine(o + dx, o + dx + dz);
        addLine(o + dy, o + dy + dx); addLine(o + dy, o + dy + dz);
        addLine(o + dz, o + dz + dx); addLine(o + dz, o + dz + dy);
        addLine(o + dx + dy, o + dx + dy + dz);
        addLine(o + dx + dz, o + dx + dy + dz);
        addLine(o + dy + dz, o + dx + dy + dz);
    }

    m_CellVertexCount = static_cast<int>(verts.size() / 6);

    glBindVertexArray(m_CellVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CellVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
    // loc0 = position, loc1 = color
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// ============================================================
// 球体 (UV Sphere): position + normal, 沿 Y 轴
// ============================================================

void LatticeDemo::GenerateSphere(std::vector<float> &verts, std::vector<unsigned int> &indices,
                                  float radius, int sectors, int stacks)
{
    for (int i = 0; i <= stacks; i++)
    {
        float phi = glm::pi<float>() * float(i) / stacks;
        float sp = std::sin(phi), cp = std::cos(phi);

        for (int j = 0; j <= sectors; j++)
        {
            float theta = 2.0f * glm::pi<float>() * float(j) / sectors;
            float x = sp * std::cos(theta);
            float y = cp;
            float z = sp * std::sin(theta);
            verts.insert(verts.end(), {x * radius, y * radius, z * radius, x, y, z});
        }
    }

    for (int i = 0; i < stacks; i++)
    {
        int row = i * (sectors + 1);
        int next = (i + 1) * (sectors + 1);
        for (int j = 0; j < sectors; j++)
        {
            if (i != 0)          { indices.insert(indices.end(), {(unsigned)row+j, (unsigned)next+j, (unsigned)row+j+1}); }
            if (i != stacks - 1) { indices.insert(indices.end(), {(unsigned)row+j+1, (unsigned)next+j, (unsigned)next+j+1}); }
        }
    }
}

// ============================================================
// 圆柱体: 沿 Y 轴, y ∈ [0,1], radius = 1
// ============================================================

void LatticeDemo::GenerateCylinder(std::vector<float> &verts, std::vector<unsigned int> &indices,
                                    float radius, int sectors)
{
    for (int i = 0; i <= 1; i++)
    {
        float y = float(i);
        for (int j = 0; j <= sectors; j++)
        {
            float theta = 2.0f * glm::pi<float>() * float(j) / sectors;
            float x = std::cos(theta), z = std::sin(theta);
            verts.insert(verts.end(), {x * radius, y, z * radius, x, 0.0f, z});
        }
    }

    for (int j = 0; j < sectors; j++)
    {
        int b = j, t = j + sectors + 1;
        indices.insert(indices.end(), {(unsigned)b, (unsigned)t, (unsigned)b+1});
        indices.insert(indices.end(), {(unsigned)b+1, (unsigned)t, (unsigned)t+1});
    }
}

void LatticeDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_Shader);
}
