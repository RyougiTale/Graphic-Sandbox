#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// ============================================================
// MeshRenderer — CPU 生成网格的 GPU 渲染器
// ============================================================
// 用于渲染 Marching Cubes 等算法产生的三角网格
// 顶点格式: position(vec3) + normal(vec3), 交错排列
//
// 使用流程:
//   1. MeshRenderer renderer;
//   2. renderer.Upload(vertices, indices);   // CPU → GPU
//   3. renderer.Draw();                       // 渲染
//   4. renderer.Destroy();                    // 释放 GPU 资源
//
// 注意: Upload 可以多次调用 (更新网格), 内部会重新分配 buffer

class MeshRenderer
{
public:
    MeshRenderer() = default;
    ~MeshRenderer() { Destroy(); }

    MeshRenderer(const MeshRenderer &) = delete;
    MeshRenderer &operator=(const MeshRenderer &) = delete;
    MeshRenderer(MeshRenderer &&other) noexcept;
    MeshRenderer &operator=(MeshRenderer &&other) noexcept;

    // 上传顶点数据 (position + normal 交错, 6 float/vertex)
    void Upload(const std::vector<float> &vertices,
                const std::vector<uint32_t> &indices);

    // 从 MarchingCubes::Vertex 格式上传
    struct MCVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
    };
    void Upload(const std::vector<MCVertex> &vertices,
                const std::vector<uint32_t> &indices);

    void Draw() const;
    void Destroy();

    bool IsValid() const { return m_VAO != 0; }
    int GetIndexCount() const { return m_IndexCount; }

private:
    void SetupVAO();

    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
    int m_IndexCount = 0;
};
