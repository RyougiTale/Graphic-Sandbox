#pragma once

#include <glm/glm.hpp>
#include <vector>

// ============================================================
// ConvexPolytope — 凸多面体的半平面裁剪
// ============================================================
//
// 核心用途: 构造 3D Voronoi 单元格
//
// Voronoi 单元格的几何定义:
//   对于种子点 P_i, 它的 Voronoi 单元格是所有"比到其他种子点更近 P_i"的点的集合:
//   V_i = { x ∈ R³ : |x - P_i| ≤ |x - P_j|, ∀j ≠ i }
//
//   每个条件 |x - P_i| ≤ |x - P_j| 定义一个半空间 (以 P_i-P_j 的中垂面为界),
//   所有半空间的交集就是一个凸多面体 — Voronoi 单元格。
//
// 算法: 增量半平面裁剪
//   1. 初始化为包围盒 (一个凸多面体)
//   2. 对于每个邻居种子 P_j:
//      a. 计算中垂面: 法线 n = normalize(P_j - P_i), 过中点 m = (P_i + P_j) / 2
//      b. 半平面条件: dot(n, x) ≤ dot(n, m)  →  保留 P_i 侧
//      c. 用 Sutherland-Hodgman 算法裁剪每个面
//      d. 收集裁剪产生的交点, 构造新的截面 (cap face)
//   3. 裁剪完成后, 剩余的凸多面体就是 V_i
//
// 复杂度: O(N × F), N = 种子数, F = 当前面数
//   实际 Voronoi 单元格通常有 10-20 个面
//
// 3D Voronoi 的对偶关系:
//   Voronoi 边 ↔ 三个种子的中垂面的交线
//   Voronoi 顶点 ↔ 四个种子的等距点 (四面体外接球心)
//   Voronoi 面 ↔ 两个相邻种子的中垂面
//   这与 Delaunay 四面体化 (Delaunay Tetrahedralization) 是对偶的
// ============================================================

class ConvexPolytope
{
public:
    // 面: 有序顶点列表 (构成凸多边形)
    struct Face
    {
        std::vector<glm::vec3> vertices;
    };

    // 边: 两个端点
    struct Edge
    {
        glm::vec3 a, b;
    };

    // ---- 构造 ----
    static ConvexPolytope MakeBox(const glm::vec3 &bmin, const glm::vec3 &bmax);

    // ---- 操作 ----
    // 半平面裁剪: 保留满足 dot(normal, x) ≤ d 的部分
    void ClipByPlane(const glm::vec3 &normal, float d);

    // ---- 查询 ----
    bool IsEmpty() const { return m_Faces.empty(); }
    const std::vector<Face> &GetFaces() const { return m_Faces; }

    // 提取去重后的边
    std::vector<Edge> ExtractEdges(float eps = 1e-4f) const;

    // 提取去重后的顶点
    std::vector<glm::vec3> ExtractVertices(float eps = 1e-4f) const;

private:
    std::vector<Face> m_Faces;

    // Sutherland-Hodgman 多边形裁剪
    // 将凸多边形 poly 裁剪到半平面 dot(normal, x) ≤ d
    static std::vector<glm::vec3> ClipPolygonByPlane(
        const std::vector<glm::vec3> &poly,
        const glm::vec3 &normal, float d);
};
