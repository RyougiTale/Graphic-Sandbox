#version 460 core

in vec2 v_TexCoord;
out vec4 FragColor;

uniform sampler2D u_Texture;

void main() {
    // 用 v_TexCoord 坐标去纹理上查颜色
    // v_TexCoord 是 0~1 的比例
    // 比如 (0.33, 0.71) = 纹理水平 33%, 垂直 71% 的位置
    // GPU 内部: 0.33 × 512 = 168.96 → 在像素 168 和 169 之间
    // GL_LINEAR → 加权平均两个像素的颜色
    // 写入用整数（精确到像素），读取用浮点比例（需要插值）——这就是 image2D 和 sampler2D 的核心区别
    FragColor = texture(u_Texture, v_TexCoord);
}
