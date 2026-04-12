#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include "Geometry/MeshRenderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// ============================================================
// Explicit Conformal Hex Mapping — Demo 11
// ============================================================
//
// 核心概念: UV 参数化 + 六角网格映射
//
// UV 参数化 (UV Parameterization):
//   将 3D 曲面展平为 2D 平面, 建立 (u, v) ↔ (x, y, z) 的映射
//   常见方法:
//   - 球面映射 (本 demo): u = atan2(z,x), v = asin(y) — 简单但极点处有畸变
//   - LSCM (Least Squares Conformal Map): 最小化角度畸变
//   - ABF (Angle Based Flattening): 保持角度不变
//   - 对于 3D 打印, 畸变小 → 晶格更均匀 → 力学性能更一致
//
// 六角网格 (Hexagonal Grid):
//   为什么用六角形?
//   - 等面积下, 六角形比四边形有更高的结构效率
//   - 蜂窝定理 (Honeycomb Conjecture, 1999 年 Hales 证明):
//     六角形是将平面分成等面积区域时周长最小的形状
//   - 在曲面上的六角网格自然产生五边形 (Euler 公式: V-E+F=2)
//     12 个五边形是拓扑必需的 (参见足球 / 碳60富勒烯)
//
// 重心坐标插值 (Barycentric Interpolation):
//   UV 空间中的点 → 找到所在三角形 → 计算重心坐标 → 插值 3D 位置
//   重心坐标 (λ₁, λ₂, λ₃): p = λ₁a + λ₂b + λ₃c, λ₁+λ₂+λ₃=1
//   这是 3D 打印中 "贴花" (decal) 和 "纹理映射" 的基础
//
// 3D 打印应用:
//   - 曲面六角外壳: 汽车/航空零件的轻量化蒙皮
//   - 建筑面板系统: 自由曲面的六角形面板分割
//   - 生物支架: 均匀孔隙的曲面支架
// ============================================================

class HexMappingDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Hex Mapping"; }
    const char *GetDescription() const override
    {
        return "Hexagonal grid mapped onto sphere via UV parameterization "
               "and barycentric interpolation.";
    }

private:
    void Regenerate();

    Shader m_WireShader;
    Shader m_SurfShader;

    MeshRenderer m_BaseMesh;

    // Hex wireframe (instanced)
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
    int m_Subdivisions = 3;
    float m_SphereRadius = 3.0f;
    float m_HexSize = 0.08f;       // UV 空间中的六角形大小
    float m_StrutRadius = 0.02f;
    float m_JointRadius = 0.03f;

    bool m_ShowHex = true;
    bool m_ShowBaseMesh = true;
    bool m_BaseMeshWireframe = true;

    float m_StrutColor[3] = {0.1f, 0.8f, 0.3f};
    float m_JointColor[3] = {0.2f, 0.9f, 0.4f};
    float m_MeshColor[3] = {0.3f, 0.4f, 0.6f};

    bool m_AutoRotate = true;
    float m_RotAngle = 0.0f;
    float m_RotSpeed = 0.3f;

    bool m_NeedRebuild = true;
    int m_HexEdgeCount = 0;
    int m_HexVertCount = 0;
};
