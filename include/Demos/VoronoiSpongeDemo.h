#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include "Geometry/MeshRenderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>

// ============================================================
// Implicit 3D Voronoi Sponge — Demo 8
// ============================================================
//
// 与 Demo 7 (显式 Voronoi Wireframe) 的对比:
//   Demo 7 (显式): CPU 构造 Voronoi 单元格 → 提取边 → 渲染圆柱/球体
//                  输出: 线框骨架 (只有边和顶点)
//   Demo 8 (隐式): CPU 计算 Worley 噪声 SDF → Marching Cubes 提取等值面
//                  输出: 实体网格 (有体积、有壁厚、可 3D 打印)
//
// 核心算法: Worley 噪声 (也叫 Cellular Noise / Voronoi Noise)
//   1968 年 Steven Worley 首次提出 (SIGGRAPH 1996 发表)
//
//   对于空间中的点 p:
//     d1 = 到最近种子点的距离
//     d2 = 到次近种子点的距离
//
//   Voronoi 泡沫 SDF:
//     f(p) = d2 - d1 - thickness
//     f < 0 → 实体 (泡沫壁)
//     f > 0 → 空气 (泡沫腔)
//
//   直觉: d2 - d1 在 Voronoi 边界处 = 0 (等距两个种子)
//         在细胞中心处最大 (远离边界)
//         减去 thickness 控制壁厚
//
// 为什么需要 Marching Cubes?
//   GPU ray marching (Demo 6) 可以实时渲染 Voronoi 泡沫
//   但 3D 打印需要实际的三角网格 (STL)
//   Marching Cubes 将隐式 SDF → 显式三角网格
//   这就是 "隐式建模 → 显式网格" 的标准管线
//
// 与 Demo 7 的互补:
//   Demo 7: 只有边骨架, 像钢丝框
//   Demo 8: 有实体壁面, 像真正的泡沫/海绵
//   实际 3D 打印中两种都有用:
//     骨架 → 轻量化支撑结构 (桁架)
//     泡沫 → 隔热/隔音/缓冲材料
// ============================================================

class VoronoiSpongeDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Voronoi Sponge"; }
    const char *GetDescription() const override
    {
        return "Implicit 3D Voronoi sponge via Worley noise + Marching Cubes. "
               "Generates printable foam/sponge geometry.";
    }

private:
    void Regenerate();

    Shader m_Shader;
    MeshRenderer m_Mesh;

    // Voronoi 参数
    int m_GridN = 3;            // NxNxN 种子网格
    float m_Jitter = 0.8f;     // 抖动量
    int m_Seed = 42;
    float m_Thickness = 0.15f;  // 泡沫壁厚
    float m_BoundsSize = 6.0f;

    // Marching Cubes 参数
    int m_Resolution = 64;      // 体素分辨率

    // 外观
    float m_Color[3] = {0.85f, 0.82f, 0.75f};
    float m_BackColor[3] = {0.4f, 0.25f, 0.15f};

    // 动画
    bool m_AutoRotate = true;
    float m_RotAngle = 0.0f;
    float m_RotSpeed = 0.3f;

    bool m_NeedRebuild = true;

    // 统计
    int m_TriCount = 0;
    int m_VertCount = 0;
    float m_BuildTimeMs = 0.0f;
};
