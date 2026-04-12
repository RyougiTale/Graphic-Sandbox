#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include "Geometry/MeshRenderer.h"
#include <glm/glm.hpp>

// ============================================================
// Implicit Mesh Dual Exoskeleton — Demo 10
// ============================================================
//
// 与 Demo 9 (显式对偶图线框) 的对比:
//   Demo 9: 对偶边 → 圆柱+球体渲染 → 线框骨架
//   Demo 10: 对偶边 → Capsule SDF → Smooth-Min 混合 → Marching Cubes
//            → 光滑的管状外骨骼, 连接处自然过渡
//
// 核心技术: Capsule SDF + Smooth-Min
//
//   Capsule SDF (胶囊 SDF):
//     给定线段 (a, b) 和半径 r:
//     sdCapsule(p, a, b, r) = |p - closest_point_on_segment(p, a, b)| - r
//     在线段周围形成一个圆角柱体
//
//   Smooth-Min (平滑最小值):
//     smin(d1, d2, k) = 普通 min 的平滑版本
//     在两个物体交汇处产生圆角过渡, 而不是尖锐的交线
//     k 越大, 过渡越圆滑
//     这就是为什么管状连接处看起来像自然生长的, 而不是焊接的
//
// 3D 打印意义:
//   SDF + Smooth-Min 是生成 "有机" 晶格的标准方法:
//   - 圆角过渡减少应力集中 (fatigue life 提高 2-5 倍)
//   - 自动保证水密 (watertight) — 无需布尔运算后处理
//   - 可以自由控制管径和过渡半径
// ============================================================

class MeshDualSmoothDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Mesh Dual Smooth"; }
    const char *GetDescription() const override
    {
        return "Smooth dual-graph exoskeleton via capsule SDF blending + Marching Cubes.";
    }

private:
    void Regenerate();

    Shader m_Shader;
    MeshRenderer m_Mesh;

    // 参数
    int m_Subdivisions = 1;       // icosphere 细分 (1=80面, 2=320面)
    float m_SphereRadius = 3.0f;
    float m_StrutRadius = 0.15f;  // capsule 半径
    float m_SmoothK = 0.3f;      // smin 平滑系数
    int m_Resolution = 64;        // MC 分辨率

    float m_Color[3] = {0.9f, 0.75f, 0.2f};
    float m_BackColor[3] = {0.5f, 0.35f, 0.1f};

    bool m_AutoRotate = true;
    float m_RotAngle = 0.0f;
    float m_RotSpeed = 0.3f;

    bool m_NeedRebuild = true;
    int m_TriCount = 0;
    float m_BuildTimeMs = 0.0f;
};
