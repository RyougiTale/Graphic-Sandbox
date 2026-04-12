#include "Geometry/IcoSphere.h"
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace IcoSphere
{

Mesh Generate(int subdivisions, float radius)
{
    Mesh mesh;

    // ---- 正二十面体 (Icosahedron) ----
    // 12 个顶点由黄金比 φ = (1+√5)/2 定义:
    // 三组互相垂直的矩形: (0, ±1, ±φ) 及其置换
    float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    mesh.vertices = {
        glm::normalize(glm::vec3(-1, t, 0)),  // 0
        glm::normalize(glm::vec3(1, t, 0)),   // 1
        glm::normalize(glm::vec3(-1, -t, 0)), // 2
        glm::normalize(glm::vec3(1, -t, 0)),  // 3
        glm::normalize(glm::vec3(0, -1, t)),  // 4
        glm::normalize(glm::vec3(0, 1, t)),   // 5
        glm::normalize(glm::vec3(0, -1, -t)), // 6
        glm::normalize(glm::vec3(0, 1, -t)),  // 7
        glm::normalize(glm::vec3(t, 0, -1)),  // 8
        glm::normalize(glm::vec3(t, 0, 1)),   // 9
        glm::normalize(glm::vec3(-t, 0, -1)), // 10
        glm::normalize(glm::vec3(-t, 0, 1)),  // 11
    };

    // 20 个三角形面
    mesh.indices = {
        0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
        1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
        3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
        4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1,
    };

    // ---- 细分 ----
    for (int s = 0; s < subdivisions; s++)
    {
        // 边 → 中点索引 缓存
        std::unordered_map<uint64_t, uint32_t> midCache;

        auto getMidpoint = [&](uint32_t a, uint32_t b) -> uint32_t
        {
            uint64_t lo = std::min(a, b), hi = std::max(a, b);
            uint64_t key = (lo << 32) | hi;
            auto it = midCache.find(key);
            if (it != midCache.end())
                return it->second;

            glm::vec3 mid = glm::normalize(
                (mesh.vertices[a] + mesh.vertices[b]) * 0.5f);
            auto idx = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back(mid);
            midCache[key] = idx;
            return idx;
        };

        std::vector<uint32_t> newIdx;
        newIdx.reserve(mesh.indices.size() * 4);

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            uint32_t v0 = mesh.indices[i];
            uint32_t v1 = mesh.indices[i + 1];
            uint32_t v2 = mesh.indices[i + 2];

            uint32_t a = getMidpoint(v0, v1);
            uint32_t b = getMidpoint(v1, v2);
            uint32_t c = getMidpoint(v2, v0);

            // 4 个子三角形
            newIdx.insert(newIdx.end(), {v0, a, c});
            newIdx.insert(newIdx.end(), {a, v1, b});
            newIdx.insert(newIdx.end(), {c, b, v2});
            newIdx.insert(newIdx.end(), {a, b, c});
        }

        mesh.indices = std::move(newIdx);
    }

    // 缩放到目标半径
    for (auto &v : mesh.vertices)
        v *= radius;

    return mesh;
}

} // namespace IcoSphere
