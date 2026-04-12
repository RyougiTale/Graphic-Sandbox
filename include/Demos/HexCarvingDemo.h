#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include "Geometry/MeshRenderer.h"
#include <glm/glm.hpp>

// ============================================================
// Implicit Triplanar Hex Carving — Demo 12
// ============================================================
//
// 与 Demo 11 (显式六角映射) 的对比:
//   Demo 11: UV 参数化 → 六角网格 → 重心坐标插值 → 线框
//            优点: 精确控制, 可用于任意曲面
//            缺点: 需要 UV 展开, 极点畸变, 拓扑复杂
//   Demo 12: 三平面投影 → 六角 SDF → 布尔差 → Marching Cubes
//            优点: 无需 UV, 自动处理任意形状, 天然水密
//            缺点: 三个投影方向的 pattern 在交汇处可能不完美对齐
//
// 核心技术:
//
// 1. 三平面投影 (Triplanar Mapping):
//    对于空间中的点 p, 将 2D pattern 从三个正交平面投影:
//    - XY 平面: pattern(p.x, p.y) → 沿 Z 方向穿透
//    - YZ 平面: pattern(p.y, p.z) → 沿 X 方向穿透
//    - XZ 平面: pattern(p.x, p.z) → 沿 Y 方向穿透
//    取三个投影的并集 (min): pattern(p) = min(f_xy, f_yz, f_xz)
//    优点: 不需要 UV 坐标, 适用于任意几何体
//
// 2. 六角形 SDF (Hexagonal Distance Field):
//    在 2D 平面上定义六角形网格的距离场:
//    - 对于点 p, 找到最近的六角中心
//    - d = 到最近中心的距离 - 孔半径
//    - d < 0 → 在孔内
//    使用轴向坐标系 (Axial Coordinates) 快速定位最近六角中心
//
// 3. CSG 布尔差 (Boolean Difference):
//    result = max(shell, -pattern)
//    - shell: 球壳 SDF (|r| - R - thickness)
//    - pattern: 三平面投影的六角孔洞
//    - max(A, -B): 从 A 中减去 B
//
// 3D 打印应用:
//   三平面六角雕刻是快速生成多孔结构的工业方法:
//   - 过滤器/筛网: 均匀孔洞的壳体
//   - 散热器: 增加表面积的穿孔壳
//   - 装饰外壳: 灯罩、花瓶等
//   - 优点: 无需复杂的曲面参数化
// ============================================================

class HexCarvingDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Hex Carving"; }
    const char *GetDescription() const override
    {
        return "Triplanar-projected hex holes carved from a spherical shell via SDF + Marching Cubes.";
    }

private:
    void Regenerate();

    Shader m_Shader;
    MeshRenderer m_Mesh;

    // Shell 参数
    float m_ShellRadius = 3.0f;
    float m_ShellThickness = 0.3f;

    // Hex pattern 参数
    float m_HexCellSize = 1.0f;    // 六角格子间距
    float m_HoleRadius = 0.35f;    // 孔洞半径

    // Marching Cubes
    int m_Resolution = 80;

    // 外观
    float m_Color[3] = {0.85f, 0.85f, 0.9f};
    float m_BackColor[3] = {0.4f, 0.4f, 0.5f};

    bool m_AutoRotate = true;
    float m_RotAngle = 0.0f;
    float m_RotSpeed = 0.3f;

    bool m_NeedRebuild = true;
    int m_TriCount = 0;
    float m_BuildTimeMs = 0.0f;
};
