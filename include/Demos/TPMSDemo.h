#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include <glad/glad.h>

// TPMS 类型 (三周期极小曲面)
enum class TPMSType
{
    Gyroid,     // 螺旋面: sin(kx)cos(ky) + sin(ky)cos(kz) + sin(kz)cos(kx)
    SchwarzP,   // Schwarz P: cos(kx) + cos(ky) + cos(kz)
    SchwarzD,   // Schwarz Diamond: sin(kx)sin(ky)sin(kz) + sin(kx)cos(ky)cos(kz) + ...
    Neovius,    // Neovius: 3(cos(kx)+cos(ky)+cos(kz)) + 4cos(kx)cos(ky)cos(kz)
    Lidinoid,   // Lidinoid: 较复杂的三角函数组合
};

// 实体化模式
enum class TPMSSolidMode
{
    Sheet,      // 片状: |F(x,y,z)| < t
    Network,    // 网状: F(x,y,z) < t
};

class TPMSDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "TPMS Lattice"; }
    const char *GetDescription() const override
    {
        return "Triply Periodic Minimal Surfaces rendered via ray marching. "
               "Gyroid, Schwarz P/D, Neovius, Lidinoid with sheet/network modes.";
    }

private:
    Shader m_Shader; // fullscreen ray marching shader
    GLuint m_VAO = 0;

    // TPMS 参数
    TPMSType m_Type = TPMSType::Gyroid;
    TPMSSolidMode m_SolidMode = TPMSSolidMode::Sheet;
    float m_CellSize = 3.0f;       // 晶胞大小 (k = 2π / cellSize)
    float m_Thickness = 0.3f;      // 壁厚 (sheet 模式的 t)
    float m_IsoValue = 0.0f;       // 等值面阈值 (network 模式的 t)

    // 渲染参数
    int m_MaxSteps = 128;          // ray marching 最大步数
    float m_MaxDist = 30.0f;       // 最大射线距离
    float m_BoundingBox = 8.0f;    // 渲染范围 (立方体半边长)

    // 外观
    float m_Color1[3] = {0.15f, 0.55f, 0.85f}; // 正面颜色
    float m_Color2[3] = {0.85f, 0.35f, 0.15f}; // 背面颜色 (sheet 模式可见两面)
    float m_BgColor[3] = {0.05f, 0.05f, 0.08f};

    // 动画
    float m_Time = 0.0f;
    bool m_Animate = false;        // 动态变形动画
    float m_AnimSpeed = 0.5f;
};
