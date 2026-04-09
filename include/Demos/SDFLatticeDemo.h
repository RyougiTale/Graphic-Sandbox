#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include <glad/glad.h>

// ============================================================
// SDF Strut Lattice — 与 LatticeDemo 的对比
// ============================================================
// LatticeDemo (mesh-based):
//   - 球体 + 圆柱实例化渲染
//   - 优点: 渲染快 (GPU 原生三角形), 容易理解
//   - 缺点: 节点处杆件直接穿插, 没有圆角, 面数随规模爆炸
//
// SDFLatticeDemo (SDF ray marching):
//   - 每根杆件是一个 Capsule SDF, 用 Smooth-Min 融合
//   - 优点: 节点自动生成完美圆角, 无拓扑错误, 无限分辨率
//   - 缺点: 渲染靠 ray marching, GPU 开销大 (逐像素迭代)
//
// 这就是 nTop 等现代隐式建模软件的核心思路:
//   传统 CAD 用显式三角面 → 连接处需要复杂的布尔运算 + 圆角修复
//   SDF 隐式建模 → Smooth-Min 天然产生圆角, 任意数量的杆件直接融合
// ============================================================

enum class SDFLatticeType
{
    SimpleCubic,    // SC: 6 条边, 沿 x/y/z 轴
    BodyCentered,   // BCC: SC + 8 条体对角线
    FaceCentered,   // FCC: 12 条面对角线
    Octet,          // Octet truss: FCC + SC 的组合, 力学性能最优
    Kelvin,         // Kelvin cell (截角八面体): 14面体, 等体积泡沫最优
};

class SDFLatticeDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "SDF Lattice"; }
    const char *GetDescription() const override
    {
        return "Strut-based lattice via SDF ray marching with Smooth-Min blending. "
               "Compare with mesh-based Lattice demo to see the difference.";
    }

private:
    Shader m_Shader;
    GLuint m_VAO = 0;

    // 晶格参数
    SDFLatticeType m_Type = SDFLatticeType::SimpleCubic;
    float m_CellSize = 3.0f;
    float m_StrutRadius = 0.15f;    // 杆件半径
    float m_NodeRadius = 0.25f;     // 节点球半径 (0=仅靠 smooth-min 产生圆角)
    float m_SmoothK = 0.15f;        // Smooth-Min 的 k 值 (控制圆角大小)
    int m_Repeat = 3;               // 每方向重复数

    // 渲染
    int m_MaxSteps = 128;
    float m_MaxDist = 40.0f;
    float m_BoundingBox = 8.0f;

    // 外观
    float m_StrutColor[3] = {0.75f, 0.75f, 0.78f}; // 金属灰
    float m_NodeColor[3] = {0.3f, 0.6f, 1.0f};     // 节点高亮
    float m_BgColor[3] = {0.05f, 0.05f, 0.08f};

    // 动画
    bool m_AutoRotate = false;
    float m_RotationAngle = 0.0f;
    float m_RotationSpeed = 0.5f;
};
