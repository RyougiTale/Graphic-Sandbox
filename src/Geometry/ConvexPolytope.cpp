#include "Geometry/ConvexPolytope.h"
#include <algorithm>
#include <cmath>

// ============================================================
// 构造包围盒
// ============================================================

ConvexPolytope ConvexPolytope::MakeBox(const glm::vec3 &bmin, const glm::vec3 &bmax)
{
    ConvexPolytope p;

    //   7----6
    //  /|   /|
    // 4----5 |
    // | 3--|-2
    // |/   |/
    // 0----1
    glm::vec3 v[8] = {
        {bmin.x, bmin.y, bmin.z}, // 0
        {bmax.x, bmin.y, bmin.z}, // 1
        {bmax.x, bmax.y, bmin.z}, // 2
        {bmin.x, bmax.y, bmin.z}, // 3
        {bmin.x, bmin.y, bmax.z}, // 4
        {bmax.x, bmin.y, bmax.z}, // 5
        {bmax.x, bmax.y, bmax.z}, // 6
        {bmin.x, bmax.y, bmax.z}, // 7
    };

    // 6 个面 (顶点按凸多边形顺序排列)
    p.m_Faces = {
        {{v[0], v[3], v[2], v[1]}}, // -Z 面
        {{v[4], v[5], v[6], v[7]}}, // +Z 面
        {{v[0], v[1], v[5], v[4]}}, // -Y 面
        {{v[2], v[3], v[7], v[6]}}, // +Y 面
        {{v[0], v[4], v[7], v[3]}}, // -X 面
        {{v[1], v[2], v[6], v[5]}}, // +X 面
    };

    return p;
}

// ============================================================
// Sutherland-Hodgman 多边形裁剪
// ============================================================
// 经典的逐边裁剪算法, 1974 年由 Sutherland 和 Hodgman 提出
// 原始版本是 2D 的, 这里推广到 3D (用平面代替直线)
//
// 对于多边形的每条边 (curr → next):
//   - curr 在内 + next 在内 → 输出 curr
//   - curr 在内 + next 在外 → 输出 curr + 交点
//   - curr 在外 + next 在内 → 输出交点
//   - curr 在外 + next 在外 → 不输出
//
// 交点计算:
//   设 dc = dot(n, curr) - d, dn = dot(n, next) - d
//   交点参数 t = dc / (dc - dn)
//   交点 = lerp(curr, next, t) = curr + t * (next - curr)

std::vector<glm::vec3> ConvexPolytope::ClipPolygonByPlane(
    const std::vector<glm::vec3> &poly,
    const glm::vec3 &normal, float d)
{
    if (poly.size() < 3)
        return {};

    const float eps = 1e-6f;
    std::vector<glm::vec3> out;
    int n = static_cast<int>(poly.size());

    for (int i = 0; i < n; i++)
    {
        const glm::vec3 &curr = poly[i];
        const glm::vec3 &next = poly[(i + 1) % n];

        float dc = glm::dot(normal, curr) - d;
        float dn = glm::dot(normal, next) - d;

        bool cIn = (dc <= eps);
        bool nIn = (dn <= eps);

        if (cIn)
        {
            out.push_back(curr);
            if (!nIn)
            {
                // 从内到外: 输出交点
                float t = dc / (dc - dn);
                out.push_back(glm::mix(curr, next, t));
            }
        }
        else if (nIn)
        {
            // 从外到内: 输出交点
            float t = dc / (dc - dn);
            out.push_back(glm::mix(curr, next, t));
        }
    }

    return out;
}

// ============================================================
// 半平面裁剪
// ============================================================
// 对多面体的每个面执行 Sutherland-Hodgman 裁剪,
// 然后收集裁剪面上的交点, 构造新的截面 (cap face)
//
// cap face 的构造:
//   所有位于裁剪平面上的点 (dot(n, v) ≈ d) 构成一个凸多边形
//   按极角排序后即为 cap face 的顶点序列

void ConvexPolytope::ClipByPlane(const glm::vec3 &normal, float d)
{
    const float onPlaneEps = 1e-5f;
    std::vector<Face> newFaces;
    std::vector<glm::vec3> capPoints;

    for (auto &face : m_Faces)
    {
        auto clipped = ClipPolygonByPlane(face.vertices, normal, d);
        if (clipped.size() < 3)
            continue;

        newFaces.push_back({clipped});

        // 收集位于裁剪面上的顶点 (用于构造 cap face)
        for (auto &v : clipped)
        {
            if (std::abs(glm::dot(normal, v) - d) < onPlaneEps)
            {
                // 去重
                bool dup = false;
                for (auto &cp : capPoints)
                {
                    if (glm::distance(v, cp) < onPlaneEps)
                    {
                        dup = true;
                        break;
                    }
                }
                if (!dup)
                    capPoints.push_back(v);
            }
        }
    }

    // 用交点构造截面 (cap face)
    if (capPoints.size() >= 3)
    {
        // 计算质心
        glm::vec3 centroid(0.0f);
        for (auto &p : capPoints)
            centroid += p;
        centroid /= static_cast<float>(capPoints.size());

        // 在裁剪平面上建立局部 2D 坐标系
        glm::vec3 ref = (std::abs(normal.y) < 0.9f)
                            ? glm::vec3(0.0f, 1.0f, 0.0f)
                            : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 u = glm::normalize(ref - glm::dot(ref, normal) * normal);
        glm::vec3 v = glm::cross(normal, u);

        // 按极角排序 (使顶点按凸多边形顺序排列)
        std::sort(capPoints.begin(), capPoints.end(),
                  [&](const glm::vec3 &a, const glm::vec3 &b)
                  {
                      glm::vec3 da = a - centroid;
                      glm::vec3 db = b - centroid;
                      return std::atan2(glm::dot(da, v), glm::dot(da, u)) <
                             std::atan2(glm::dot(db, v), glm::dot(db, u));
                  });

        newFaces.push_back({capPoints});
    }

    m_Faces = newFaces;
}

// ============================================================
// 提取唯一边
// ============================================================

std::vector<ConvexPolytope::Edge> ConvexPolytope::ExtractEdges(float eps) const
{
    std::vector<Edge> edges;

    for (auto &face : m_Faces)
    {
        int n = static_cast<int>(face.vertices.size());
        for (int i = 0; i < n; i++)
        {
            const glm::vec3 &a = face.vertices[i];
            const glm::vec3 &b = face.vertices[(i + 1) % n];

            // 检查边是否已存在 (任一方向)
            bool found = false;
            for (auto &e : edges)
            {
                if ((glm::distance(e.a, a) < eps && glm::distance(e.b, b) < eps) ||
                    (glm::distance(e.a, b) < eps && glm::distance(e.b, a) < eps))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                edges.push_back({a, b});
        }
    }

    return edges;
}

// ============================================================
// 提取唯一顶点
// ============================================================

std::vector<glm::vec3> ConvexPolytope::ExtractVertices(float eps) const
{
    std::vector<glm::vec3> verts;

    for (auto &face : m_Faces)
    {
        for (auto &v : face.vertices)
        {
            bool found = false;
            for (auto &ev : verts)
            {
                if (glm::distance(ev, v) < eps)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                verts.push_back(v);
        }
    }

    return verts;
}
