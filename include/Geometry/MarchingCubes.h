#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>

// ============================================================
// Marching Cubes — 等值面提取算法
// ============================================================
//
// 历史:
//   1987 年由 Lorensen & Cline 发表于 SIGGRAPH
//   原始论文: "Marching Cubes: A High Resolution 3D Surface Construction Algorithm"
//   是计算机图形学中最广泛使用的等值面提取算法之一
//
// 核心思想:
//   给定一个 3D 标量场 f(x,y,z) 和等值 isoLevel:
//   1. 将空间划分为规则立方体网格
//   2. 对每个立方体的 8 个顶点, 判断在等值面的内侧 (f < iso) 还是外侧
//   3. 8 个顶点, 每个有 2 种状态 → 2⁸ = 256 种情况
//   4. 用查找表 (LUT) 确定等值面如何穿过每个立方体
//   5. 在立方体边上插值, 精确定位等值面与边的交点
//
// 为什么 3D 打印需要 Marching Cubes?
//   隐式建模 (SDF, 距离场, 密度场) 是晶格设计的主流方法:
//   - 可以用简单公式定义复杂结构 (TPMS, Voronoi, 布尔运算)
//   - 自动保证拓扑正确 (无自相交)
//   - 但 3D 打印机需要三角网格 (STL/OBJ)
//   → Marching Cubes 就是从隐式表示到显式网格的桥梁
//
// 复杂度: O(N³), N = 每轴体素数
//   N=64:  26万个立方体, ~10ms
//   N=128: 200万个立方体, ~100ms
//   N=256: 1600万个立方体, ~1s
// ============================================================

namespace MarchingCubes
{

// 顶点: 位置 + 法线
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

// 等值面提取结果
struct Result
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

// 从 3D 标量场提取等值面
//
// field:    标量场函数 f(x, y, z) → float
//           f < isoLevel 视为内部 (实体), f > isoLevel 视为外部 (空气)
// gridMin:  网格最小角
// gridMax:  网格最大角
// resolution: 每轴体素数
// isoLevel: 等值面阈值 (默认 0, 适用于 SDF)
Result Extract(
    const std::function<float(float, float, float)> &field,
    const glm::vec3 &gridMin,
    const glm::vec3 &gridMax,
    int resolution,
    float isoLevel = 0.0f);

} // namespace MarchingCubes
