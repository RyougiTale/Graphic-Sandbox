#version 460 core

// 从顶点着色器传来, 已经过光栅化插值
in vec3 v_Color;

// CPU 传入的染色颜色, 与顶点颜色相乘
uniform vec3 u_TintColor;

// 片段最终输出颜色 (RGBA)
out vec4 FragColor;

void main() {
    // 逐分量相乘: (r1*r2, g1*g2, b1*b2), alpha 固定为 1.0 (不透明)
    FragColor = vec4(v_Color * u_TintColor, 1.0);
}
