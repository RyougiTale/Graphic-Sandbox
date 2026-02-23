#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec3 v_Color;

void main() {
    // 没有 Model 矩阵, 顶点直接在世界空间
    gl_Position = u_Projection * u_View * vec4(a_Position, 1.0);
    v_Color = a_Color;
}
