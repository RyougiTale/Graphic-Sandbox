#version 460 core

// 模板 mesh 顶点
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;   // 法线 (球体/圆柱) 或 颜色 (线框)

// 实例数据 — 统一布局: 3 个 vec3
// 球体模式 (mode 0): a = position, b = color, c = unused
// 圆柱模式 (mode 1): a = from,     b = to,    c = color
layout(location = 2) in vec3 a_InstA;
layout(location = 3) in vec3 a_InstB;
layout(location = 4) in vec3 a_InstC;

uniform mat4  u_Model;
uniform mat4  u_View;
uniform mat4  u_Projection;
uniform int   u_RenderMode;  // 0 = 球体, 1 = 圆柱, 2 = 线框
uniform float u_AtomRadius;  // 球体半径 (mode 0)
uniform float u_BondRadius;  // 圆柱半径 (mode 1)

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec3 v_Color;
flat out int v_Mode;

void main()
{
    vec3 localPos;
    vec3 normal;
    vec3 color;

    if (u_RenderMode == 0)
    {
        // 球体: 缩放模板, 平移到实例位置
        localPos = a_Position * u_AtomRadius + a_InstA;
        normal   = a_Normal;
        color    = a_InstB;
    }
    else if (u_RenderMode == 1)
    {
        // 圆柱: 将 Y 轴单位圆柱变换到 from → to
        vec3 from = a_InstA;
        vec3 to   = a_InstB;
        color     = a_InstC;

        vec3 dir    = to - from;
        float len   = length(dir);
        vec3 up     = (len > 0.001) ? dir / len : vec3(0.0, 1.0, 0.0);

        // 构造正交基
        vec3 arb    = (abs(up.y) < 0.99) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 right  = normalize(cross(up, arb));
        vec3 fwd    = cross(right, up);

        localPos = from
                 + right * (a_Position.x * u_BondRadius)
                 + up    * (a_Position.y * len)
                 + fwd   * (a_Position.z * u_BondRadius);

        normal = normalize(right * a_Normal.x + up * a_Normal.y + fwd * a_Normal.z);
    }
    else
    {
        // 线框: loc1 存的是颜色
        localPos = a_Position;
        normal   = vec3(0.0, 1.0, 0.0);
        color    = a_Normal;
    }

    vec4 worldPos4 = u_Model * vec4(localPos, 1.0);
    v_WorldPos    = worldPos4.xyz;
    v_WorldNormal = mat3(transpose(inverse(u_Model))) * normal;
    v_Color       = color;
    v_Mode        = u_RenderMode;

    gl_Position = u_Projection * u_View * worldPos4;
}
