#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// ============================================================
// Explicit 3D Voronoi Wireframe — Demo 7
// ============================================================
//
// 与前面 VoronoiLatticeDemo (Demo 6, GPU ray marching) 的对比:
//   Demo 6: 在 shader 中实时计算 Voronoi 距离场, 用 ray marching 渲染
//           → 纯视觉效果, 没有实际的几何数据, 无法导出
//   Demo 7: 在 CPU 上构造实际的 Voronoi 几何 (顶点 + 边)
//           → 可以导出为 STL/OBJ 网格, 用于 3D 打印
//
// 核心算法: 半平面裁剪 (Half-Plane Clipping)
//   1. 在空间中放置种子点 (抖动网格, 可控随机性)
//   2. 对每个种子点 P_i:
//      a. 从包围盒开始 (凸多面体)
//      b. 对每个其他种子 P_j, 用 P_i-P_j 的中垂面裁剪
//      c. 保留更靠近 P_i 的那一侧
//   3. 从所有单元格中提取唯一的边和顶点
//   4. 用实例化圆柱体 (strut) 和球体 (joint) 渲染
//
// 3D 打印应用:
//   Voronoi wireframe 是最常见的随机晶格结构之一:
//   - 骨科植入物: 模拟松质骨的多孔结构, 促进骨长入 (osseointegration)
//   - 轻量化零件: 内部填充 Voronoi, 减重 40-70% 同时保持 70-90% 强度
//   - 散热器: 随机通道增强流体混合和换热效率
//   - 建筑/艺术: Voronoi 泡沫的自然美感
//
// 复杂度: O(N² × F), N = 种子数, F = 平均面数/cell
//   对于 N ≤ 216 (6³), 几乎瞬间完成
// ============================================================

class VoronoiWireframeDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Voronoi Wireframe"; }
    const char *GetDescription() const override
    {
        return "Explicit 3D Voronoi wireframe via half-plane clipping. "
               "Constructs actual geometry for 3D printing applications.";
    }

private:
    void GenerateVoronoi();

    Shader m_Shader;

    // 球体模板 mesh (joints 和 seeds 共用 VBO/EBO, 各自有独立 VAO + 实例 VBO)
    GLuint m_SphereVBO = 0, m_SphereEBO = 0;
    int m_SphereIndexCount = 0;

    // 圆柱模板 mesh (struts)
    GLuint m_CylVBO = 0, m_CylEBO = 0;
    int m_CylIndexCount = 0;

    // Voronoi 顶点 — joints (球体)
    GLuint m_JointVAO = 0, m_JointInstVBO = 0;
    int m_JointCount = 0;

    // 种子点 — seeds (球体, 较大, 不同颜色)
    GLuint m_SeedVAO = 0, m_SeedInstVBO = 0;
    int m_SeedCount = 0;

    // Voronoi 边 — struts (圆柱体)
    GLuint m_StrutVAO = 0, m_StrutInstVBO = 0;
    int m_StrutCount = 0;

    // 包围盒线框
    GLuint m_BoxVAO = 0, m_BoxVBO = 0;
    int m_BoxVertCount = 0;

    // 实例数据布局 (与 LatticeDemo 相同):
    //   球体: a = position, b = color, c = unused
    //   圆柱: a = from, b = to, c = color
    struct InstanceData
    {
        glm::vec3 a, b, c;
    };
    std::vector<InstanceData> m_JointInstances;
    std::vector<InstanceData> m_StrutInstances;
    std::vector<InstanceData> m_SeedInstances;

    // 种子参数
    int m_GridN = 3;           // NxNxN 抖动网格
    float m_Jitter = 0.8f;    // 0 = 规则网格, 1 = 完全随机
    int m_Seed = 42;
    float m_BoundsSize = 8.0f;

    // 几何参数
    float m_StrutRadius = 0.05f;
    float m_JointRadius = 0.08f;
    float m_SeedRadius = 0.15f;

    // 可见性
    bool m_ShowStruts = true;
    bool m_ShowJoints = true;
    bool m_ShowSeeds = true;
    bool m_ShowBounds = true;

    // 颜色
    float m_StrutColor[3] = {0.85f, 0.82f, 0.75f};  // 骨白色
    float m_JointColor[3] = {0.9f, 0.85f, 0.7f};
    float m_SeedColor[3] = {0.2f, 0.6f, 1.0f};      // 蓝色种子点
    float m_BoundsColor[3] = {0.4f, 0.4f, 0.4f};

    // 动画
    bool m_AutoRotate = true;
    float m_RotAngle = 0.0f;
    float m_RotSpeed = 0.3f;

    bool m_NeedRebuild = true;

    // 统计
    int m_NumEdges = 0;
    int m_NumVertices = 0;
};
