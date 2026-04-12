#include "Geometry/Primitives.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace Primitives
{

// ============================================================
// UV 球体
// ============================================================
// 用经纬线参数化生成球面:
//   phi ∈ [0, π]  (从北极到南极, stacks 层)
//   theta ∈ [0, 2π] (绕赤道一圈, sectors 段)
//   x = sin(phi) * cos(theta)
//   y = cos(phi)
//   z = sin(phi) * sin(theta)
// 法线 = 归一化位置 (对于单位球, 法线 = 顶点方向)

void GenerateSphere(std::vector<float> &verts, std::vector<unsigned int> &indices,
                    float radius, int sectors, int stacks)
{
    for (int i = 0; i <= stacks; i++)
    {
        float phi = glm::pi<float>() * static_cast<float>(i) / stacks;
        float sp = std::sin(phi), cp = std::cos(phi);

        for (int j = 0; j <= sectors; j++)
        {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(j) / sectors;
            float x = sp * std::cos(theta);
            float y = cp;
            float z = sp * std::sin(theta);
            verts.insert(verts.end(), {x * radius, y * radius, z * radius, x, y, z});
        }
    }

    for (int i = 0; i < stacks; i++)
    {
        int row = i * (sectors + 1);
        int next = (i + 1) * (sectors + 1);
        for (int j = 0; j < sectors; j++)
        {
            if (i != 0)
                indices.insert(indices.end(),
                               {static_cast<unsigned>(row + j),
                                static_cast<unsigned>(next + j),
                                static_cast<unsigned>(row + j + 1)});
            if (i != stacks - 1)
                indices.insert(indices.end(),
                               {static_cast<unsigned>(row + j + 1),
                                static_cast<unsigned>(next + j),
                                static_cast<unsigned>(next + j + 1)});
        }
    }
}

// ============================================================
// 圆柱体 (无盖)
// ============================================================
// 沿 Y 轴, y ∈ [0, 1], 侧面法线指向外
// 在 vertex shader 中, 通过 from→to 向量将 Y 轴对齐到任意方向:
//   up = normalize(to - from)
//   right = normalize(cross(up, arbitrary))
//   fwd = cross(right, up)
//   worldPos = from + right * x * radius + up * y * length + fwd * z * radius

void GenerateCylinder(std::vector<float> &verts, std::vector<unsigned int> &indices,
                      float radius, int sectors)
{
    for (int i = 0; i <= 1; i++)
    {
        float y = static_cast<float>(i);
        for (int j = 0; j <= sectors; j++)
        {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(j) / sectors;
            float x = std::cos(theta), z = std::sin(theta);
            verts.insert(verts.end(), {x * radius, y, z * radius, x, 0.0f, z});
        }
    }

    for (int j = 0; j < sectors; j++)
    {
        int b = j, t = j + sectors + 1;
        indices.insert(indices.end(),
                       {static_cast<unsigned>(b),
                        static_cast<unsigned>(t),
                        static_cast<unsigned>(b + 1)});
        indices.insert(indices.end(),
                       {static_cast<unsigned>(b + 1),
                        static_cast<unsigned>(t),
                        static_cast<unsigned>(t + 1)});
    }
}

} // namespace Primitives
