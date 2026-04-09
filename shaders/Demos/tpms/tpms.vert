#version 460 core

// 全屏三角形 — 与 compute demo 相同的技巧
// 用一个超大三角形覆盖整个屏幕, 不需要 VBO

out vec2 v_UV; // 屏幕 UV: (0,0)=左下, (1,1)=右上

void main()
{
    vec2 pos[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    // NDC [-1,1] → UV [0,1]
    v_UV = pos[gl_VertexID] * 0.5 + 0.5;
}
