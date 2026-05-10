#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include "Geometry/MeshRenderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

enum class StructuralBuildMode
{
    GraphStruts,
    VoronoiStruts,
    TPMSSurface,
    VoronoiFoam
};

// Structural lattice: unit cell + cell map + SDF domain + thickness field.
enum class StructuralCellType
{
    Cubic,
    BodyCentered,
    Octet,
    Diamond
};

enum class StructuralDomainType
{
    Box,
    Sphere,
    ShoeSole
};

enum class StructuralFieldType
{
    Uniform,
    BoundaryDense,
    VerticalGradient,
    CenterDense
};

enum class StructuralTPMSType
{
    Gyroid,
    Diamond
};

class StructuralLatticeDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Structural Lattice"; }
    const char *GetDescription() const override
    {
        return "Field-driven structural lattice clipped by simple SDF design domains.";
    }

private:
    struct InstanceData
    {
        glm::vec3 a; // strut: from, joint: position
        glm::vec3 b; // strut: to,   joint: unused
        glm::vec3 c; // color
        float radius;
    };

    struct CellEdge
    {
        glm::vec3 a;
        glm::vec3 b;
    };

    void RebuildLattice();
    void UploadDomainWireframe();
    void UploadInstances();
    void RebuildGraphLattice();
    void RebuildVoronoiStruts();
    void RebuildImplicitLattice();

    std::vector<CellEdge> BuildUnitCell() const;
    bool ClipSegmentToDomain(const glm::vec3 &p0, const glm::vec3 &p1,
                             glm::vec3 &outA, glm::vec3 &outB) const;
    float DomainSDF(const glm::vec3 &p) const;
    float RadiusAt(const glm::vec3 &p) const;
    float SoleHalfWidthAt(float x) const;
    float TPMSField(const glm::vec3 &p) const;
    float TPMSLatticeSDF(const glm::vec3 &p) const;
    float VoronoiLatticeSDF(const glm::vec3 &p) const;

    Shader m_Shader;
    Shader m_SurfaceShader;
    MeshRenderer m_ImplicitMesh;

    GLuint m_SphereVBO = 0, m_SphereEBO = 0;
    GLuint m_CylVBO = 0, m_CylEBO = 0;
    int m_SphereIndexCount = 0;
    int m_CylIndexCount = 0;

    GLuint m_JointVAO = 0, m_JointInstVBO = 0;
    GLuint m_StrutVAO = 0, m_StrutInstVBO = 0;
    GLuint m_DomainVAO = 0, m_DomainVBO = 0;
    int m_JointCount = 0;
    int m_StrutCount = 0;
    int m_DomainVertCount = 0;

    std::vector<InstanceData> m_JointInstances;
    std::vector<InstanceData> m_StrutInstances;

    StructuralBuildMode m_BuildMode = StructuralBuildMode::GraphStruts;
    StructuralCellType m_CellType = StructuralCellType::Octet;
    StructuralDomainType m_DomainType = StructuralDomainType::ShoeSole;
    StructuralFieldType m_FieldType = StructuralFieldType::BoundaryDense;
    StructuralTPMSType m_TPMSType = StructuralTPMSType::Gyroid;

    glm::vec3 m_DomainSize = glm::vec3(7.0f, 1.8f, 3.2f);
    float m_CellSize = 0.65f;
    float m_StrutRadius = 0.045f;
    float m_JointScale = 1.35f;
    float m_FieldStrength = 0.75f;
    float m_FieldBand = 0.65f;
    float m_TPMSThickness = 0.16f;
    float m_TPMSIsoValue = 0.0f;
    float m_VoronoiThickness = 0.12f;
    float m_VoronoiJitter = 0.75f;
    int m_VoronoiSeed = 42;
    int m_VoronoiNeighborCount = 4;
    int m_ImplicitResolution = 96;

    bool m_ShowStruts = true;
    bool m_ShowJoints = true;
    bool m_ShowDomain = true;
    bool m_AutoRotate = true;
    float m_RotAngle = 0.0f;
    float m_RotSpeed = 0.35f;

    float m_StrutColor[3] = {0.84f, 0.78f, 0.64f};
    float m_JointColor[3] = {0.95f, 0.86f, 0.58f};
    float m_DomainColor[3] = {0.25f, 0.55f, 0.95f};

    bool m_NeedRebuild = true;
    bool m_Truncated = false;
    int m_NumCells = 0;
    int m_NumUnitEdges = 0;
    int m_MaxStruts = 12000;
    int m_ImplicitTriCount = 0;
    int m_ImplicitVertCount = 0;
};
