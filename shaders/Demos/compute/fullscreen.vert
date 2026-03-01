#version 460 core

out vec2 v_TexCoord;

void main() {
    //     (-1, 3)
    //        |\
    //        | \
    //        |  \
    // (-1,1) +---+ (1,1)    ← 屏幕右上角
    //        |   |\ 
    //        |   | \         ← 三角形延伸到屏幕外
    //        |   |  \
    // (-1,-1)+---+---+ (3,-1)
    //        屏幕    屏幕外
    // NDC 坐标, 屏幕空间 -1 到 +1
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    // texCoords — 纹理坐标
    // (0,0) → 屏幕左下
    // (2,0) → 超出右边
    // (0,2) → 超出上边

    vec2 texCoords[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    // gl_VertexID
    // GPU 内置变量，当前是第几个顶点（0/1/2）。不需要 VBO，顶点数据直接写死在 shader 里
    // 0是z深度(全屏平面不需要深度), 1是w分量, 不做透视除法
    // gl_Position
    // 也是内置变量, GPU自动读取
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    // out 变量，传给 fragment shader。GPU 会在三角形内部自动做线性插值，每个像素收到自己对应的纹理坐标
    v_TexCoord = texCoords[gl_VertexID];
}
