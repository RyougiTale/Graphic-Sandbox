#version 460 core

// 顶点属性输入, location 对应 glVertexAttribPointer 的第一个参数
layout(location = 0) in vec3 a_Position; // 从 VBO 读取的顶点位置 (模型空间)
layout(location = 1) in vec3 a_Color;    // 从 VBO 读取的顶点颜色

// uniform: CPU 通过 glUniform/Shader::SetMat4 传入, 每帧可变, 所有顶点共享同一个值
// 实际上u_Model是缩放+旋转
uniform mat4 u_Model;      // 模型矩阵: 模型空间 → 世界空间 (位移/旋转/缩放)
// u_View是相机位置和朝向的逆变换
uniform mat4 u_View;       // 视图矩阵: 世界空间 → 相机空间
// u_Projection透视投影矩阵
uniform mat4 u_Projection; // 投影矩阵: 相机空间 → 裁剪空间

// out: 传给片段着色器, 光栅化阶段会自动做重心插值
out vec3 v_Color;

void main() {
    // MVP 变换: 模型空间 → 世界 → 相机 → 裁剪空间
    // 注意矩阵乘法从右往左读: 先 Model, 再 View, 最后 Projection
    // 等价于vec4(a_Position.x, a_Position.y, a_Position.z, 1.0)
    // w 区分点和方向
    // w = 1.0 → 这是一个点（位置），平移对它生效
    // w = 0.0 → 这是一个方向（向量），平移对它无效
    gl_Position = u_Projection * u_View * u_Model * vec4(a_Position, 1.0);
    // 颜色直接传递, 光栅化会在三个顶点之间插值出渐变色
    v_Color = a_Color;
}
