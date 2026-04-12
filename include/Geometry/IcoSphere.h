#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

// ============================================================
// IcoSphere — 二十面体细分球
// ============================================================
// 与 UV 球体不同, icosphere 的三角形面积分布更均匀:
//   UV 球体: 极点处三角形退化为三角形条, 面积不均
//   IcoSphere: 从正二十面体细分, 每个三角形面积接近相等
//
// 细分过程:
//   Level 0: 正二十面体 — 12 顶点, 20 面
//   Level 1: 每个三角形 → 4 个子三角形 — 42 顶点, 80 面
//   Level 2: 再次细分 — 162 顶点, 320 面
//   Level 3: — 642 顶点, 1280 面
//
// 每次细分:
//   1. 对每条边取中点
//   2. 将中点投影到单位球面 (归一化)
//   3. 用中点将原三角形分成 4 个子三角形
//   4. 用 edge→midpoint 缓存避免重复创建
//
// 公式: V = 10 × 4^n + 2, F = 20 × 4^n, E = 30 × 4^n
// ============================================================

namespace IcoSphere
{

struct Mesh
{
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices; // 三角形 (每 3 个一组)
};

Mesh Generate(int subdivisions = 2, float radius = 1.0f);

} // namespace IcoSphere
