#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// 晶格类型
enum class LatticeType
{
    SimpleCubic,    // 简单立方 (SC)
    BodyCentered,   // 体心立方 (BCC)
    FaceCentered,   // 面心立方 (FCC)
    Hexagonal,      // 六方密排 (HCP近似)
    Diamond,        // 金刚石结构
};

class LatticeDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Lattice"; }
    const char *GetDescription() const override
    {
        return "3D crystal lattice visualization. Supports SC, BCC, FCC, HCP, Diamond structures.";
    }

private:
    void RebuildLattice();
    void GenerateSphere(std::vector<float> &verts, std::vector<unsigned int> &indices,
                        float radius, int sectors, int stacks);
    void GenerateCylinder(std::vector<float> &verts, std::vector<unsigned int> &indices,
                          float radius, int sectors);
    void UploadAtomMesh();
    void UploadBondMesh();
    void UploadCellMesh();

    Shader m_Shader;

    // 原子 (球体实例化渲染)
    GLuint m_AtomVAO = 0, m_AtomVBO = 0, m_AtomEBO = 0;
    GLuint m_AtomInstanceVBO = 0;
    int m_SphereIndexCount = 0;
    int m_AtomCount = 0;

    // 键 (圆柱体实例化渲染)
    GLuint m_BondVAO = 0, m_BondVBO = 0, m_BondEBO = 0;
    GLuint m_BondInstanceVBO = 0;
    int m_CylinderIndexCount = 0;
    int m_BondCount = 0;

    // 单元格线框
    GLuint m_CellVAO = 0, m_CellVBO = 0;
    int m_CellVertexCount = 0;

    // 实例数据 — 统一布局: 3 个 vec3 per instance
    // 原子: (position, color, unused)
    // 键:   (from, to, color)
    struct InstanceData
    {
        glm::vec3 a; // 原子: position, 键: from
        glm::vec3 b; // 原子: color,    键: to
        glm::vec3 c; // 原子: unused,   键: color
    };
    std::vector<InstanceData> m_AtomInstances;
    std::vector<InstanceData> m_BondInstances;

    // 参数
    LatticeType m_LatticeType = LatticeType::SimpleCubic;
    int m_RepeatX = 3, m_RepeatY = 3, m_RepeatZ = 3;
    float m_LatticeConstant = 2.0f;
    float m_AtomRadius = 0.2f;
    float m_BondRadius = 0.05f;
    float m_BondCutoff = 1.05f; // 键长阈值 (相对于最近邻距离)
    bool m_ShowBonds = true;
    bool m_ShowUnitCell = true;
    bool m_AutoRotate = false;
    float m_RotationAngle = 0.0f;
    float m_RotationSpeed = 0.5f;

    float m_AtomColor1[3] = {0.2f, 0.6f, 1.0f};
    float m_AtomColor2[3] = {1.0f, 0.4f, 0.2f};
    float m_BondColor[3] = {0.7f, 0.7f, 0.7f};
    float m_CellColor[3] = {1.0f, 1.0f, 0.0f};

    bool m_NeedRebuild = true;
};
