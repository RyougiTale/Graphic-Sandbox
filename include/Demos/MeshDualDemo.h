#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include "Geometry/MeshRenderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// ============================================================
// Explicit Mesh Dual Graph — Demo 9
// ============================================================
//
// 对偶图 (Dual Graph) 的定义:
//   给定一个三角网格, 它的对偶图是:
//   - 每个三角形 → 一个节点 (位于三角形重心)
//   - 每条共享边 → 一条对偶边 (连接两个相邻三角形的重心)
//
// 对偶图的几何意义:
//   三角网格的对偶通常是六边形+五边形的网格:
//   - 度为 6 的顶点 → 六边形 (大部分)
//   - 度为 5 的顶点 → 五边形 (拓扑需要, 参见 Euler 公式)
//   例: 足球 (soccer ball) 就是二十面体对偶 — 12 个五边形 + 20 个六边形
//
// 与 Voronoi 的关系:
//   Delaunay 三角化的对偶 = Voronoi 图
//   三角网格的对偶 = 类 Voronoi 结构 (但不是精确的 Voronoi)
//
// 3D 打印应用:
//   对偶图形成的六角网格是常见的:
//   - 蜂窝结构: 最大刚度/重量比
//   - 外骨骼: 包裹在曲面上的轻量化骨架
//   - 建筑外墙: Voronoi/六角面板系统
//
// 算法:
//   1. 生成 icosphere (三角网格)
//   2. 构建 edge→triangle 邻接表
//   3. 计算每个三角形的重心
//   4. 连接共享边的三角形重心 → 对偶边
//   5. 用实例化圆柱+球体渲染
// ============================================================

class MeshDualDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Mesh Dual Graph"; }
    const char *GetDescription() const override
    {
        return "Explicit dual graph of icosphere. Shows hex/pentagon wireframe "
               "formed by connecting adjacent triangle barycenters.";
    }

private:
    void Regenerate();

    // Wireframe shader (for dual graph)
    Shader m_WireShader;
    // Surface shader (for base mesh overlay)
    Shader m_SurfShader;

    // Base mesh (icosphere)
    MeshRenderer m_BaseMesh;

    // Dual graph: sphere mesh for joints, cylinder mesh for struts
    GLuint m_SphereVBO = 0, m_SphereEBO = 0;
    int m_SphereIdxCount = 0;
    GLuint m_CylVBO = 0, m_CylEBO = 0;
    int m_CylIdxCount = 0;

    GLuint m_JointVAO = 0, m_JointInstVBO = 0;
    int m_JointCount = 0;
    GLuint m_StrutVAO = 0, m_StrutInstVBO = 0;
    int m_StrutCount = 0;

    struct InstanceData { glm::vec3 a, b, c; };
    std::vector<InstanceData> m_JointInst;
    std::vector<InstanceData> m_StrutInst;

    // 参数
    int m_Subdivisions = 2;
    float m_SphereRadius = 3.0f;
    float m_StrutRadius = 0.03f;
    float m_JointRadius = 0.05f;

    bool m_ShowDual = true;
    bool m_ShowBaseMesh = true;
    bool m_BaseMeshWireframe = true;

    float m_StrutColor[3] = {0.9f, 0.75f, 0.2f};
    float m_JointColor[3] = {1.0f, 0.85f, 0.3f};
    float m_MeshColor[3] = {0.3f, 0.5f, 0.8f};

    bool m_AutoRotate = true;
    float m_RotAngle = 0.0f;
    float m_RotSpeed = 0.3f;

    bool m_NeedRebuild = true;
    int m_DualEdgeCount = 0;
    int m_DualNodeCount = 0;
};
