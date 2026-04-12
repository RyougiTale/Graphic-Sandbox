#pragma once

#include <vector>

// ============================================================
// 基础几何体生成器
// ============================================================
// 生成的顶点格式: position(vec3) + normal(vec3), 交错排列 (6 float/vertex)
// 用于实例化渲染的模板 mesh
//
// 为什么不在 shader 中生成?
//   对于规则几何体 (球、柱), 在 CPU 端生成模板 mesh + 实例化渲染
//   比在 shader 中用 SDF ray marching 更快 (因为模板 mesh 只有几百个三角形,
//   而 ray marching 每像素需要几十次距离函数求值)
//   缺点是模板是多面体近似, 边缘不完全光滑

namespace Primitives
{

// UV 球体
// radius: 半径, sectors: 经线数, stacks: 纬线数
// 生成 (stacks+1) × (sectors+1) 个顶点
void GenerateSphere(std::vector<float> &verts, std::vector<unsigned int> &indices,
                    float radius = 1.0f, int sectors = 16, int stacks = 12);

// 圆柱体 (无盖)
// 沿 Y 轴, y ∈ [0, 1], radius = 半径
// 在 vertex shader 中通过 from→to 向量变换到任意方向
void GenerateCylinder(std::vector<float> &verts, std::vector<unsigned int> &indices,
                      float radius = 1.0f, int sectors = 8);

} // namespace Primitives
