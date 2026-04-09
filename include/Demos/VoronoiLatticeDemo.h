#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include <glad/glad.h>

// ============================================================
// Voronoi / Stochastic Foam Lattice
// ============================================================
// 前面的 Strut-based 和 TPMS 晶格都是"规则"的 — 周期性重复的单元格
// Voronoi 晶格是"随机"的 — 模拟自然界中的泡沫、骨骼、海绵等不规则结构
//
// 核心思想:
//   1. 在空间中撒一些随机种子点 (feature points)
//   2. 对于空间中任意一点 p, 找到离它最近和次近的种子点
//   3. 用这两个距离的差值构造 SDF:
//      d2 - d1 → 在 Voronoi 边界 (两个区域等距处) 为 0
//      取 d2 - d1 < thickness → 生成泡沫壁
//
// 与其他晶格的对比:
//   TPMS:        隐式函数 F(x,y,z) → 三角函数 → 连续、光滑、无限
//   SDF Strut:   节点+边的图 → capsule SDF + smooth-min → 规则、可控
//   Voronoi:     随机种子点 → 距离场 → 有机、不规则、自然
// ============================================================

// Voronoi 变体
enum class VoronoiType
{
    StandardFoam,   // 标准泡沫: 用 d2-d1 生成细胞壁
    ThickWall,      // 厚壁泡沫: 用 d1 > threshold 掏空每个细胞中心
    Skeleton,       // 骨架结构: 只保留 Voronoi 棱 (三个区域交汇线)
    Smooth,         // 平滑泡沫: 对细胞壁施加平滑处理
};

class VoronoiLatticeDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Voronoi Lattice"; }
    const char *GetDescription() const override
    {
        return "Stochastic foam lattice via 3D Voronoi distance fields. "
               "Simulates bone, sponge, and metal foam structures.";
    }

private:
    Shader m_Shader;
    GLuint m_VAO = 0;

    // Voronoi 参数
    VoronoiType m_Type = VoronoiType::StandardFoam;
    float m_CellScale = 2.5f;       // 细胞大小 (越大, 泡沫越粗)
    float m_WallThickness = 0.08f;   // 壁厚
    float m_Randomness = 1.0f;       // 种子点偏移随机度 (0=规则网格, 1=完全随机)
    int m_Seed = 42;                 // 随机种子

    // 渲染
    int m_MaxSteps = 128;
    float m_MaxDist = 30.0f;
    float m_BoundingBox = 8.0f;

    // 外观
    float m_WallColor[3] = {0.85f, 0.82f, 0.75f};  // 骨白色
    float m_CavityColor[3] = {0.3f, 0.15f, 0.1f};  // 骨髓深色
    float m_BgColor[3] = {0.05f, 0.05f, 0.08f};

    // 动画
    bool m_AutoRotate = false;
    float m_RotationAngle = 0.0f;
    float m_RotationSpeed = 0.5f;
    bool m_Animate = false;          // 种子点动态漂移
    float m_AnimSpeed = 0.3f;
    float m_Time = 0.0f;
};
