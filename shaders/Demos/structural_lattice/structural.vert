#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

layout(location = 2) in vec3 a_InstA;
layout(location = 3) in vec3 a_InstB;
layout(location = 4) in vec3 a_InstC;
layout(location = 5) in float a_InstRadius;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform int u_RenderMode; // 0 = joint sphere, 1 = strut cylinder, 2 = domain wire

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
        localPos = a_Position * a_InstRadius + a_InstA;
        normal = a_Normal;
        color = a_InstC;
    }
    else if (u_RenderMode == 1)
    {
        vec3 from = a_InstA;
        vec3 to = a_InstB;
        color = a_InstC;

        vec3 dir = to - from;
        float len = length(dir);
        vec3 up = (len > 0.001) ? dir / len : vec3(0.0, 1.0, 0.0);
        vec3 arb = (abs(up.y) < 0.99) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 right = normalize(cross(up, arb));
        vec3 fwd = cross(right, up);

        localPos = from
                 + right * (a_Position.x * a_InstRadius)
                 + up * (a_Position.y * len)
                 + fwd * (a_Position.z * a_InstRadius);
        normal = normalize(right * a_Normal.x + up * a_Normal.y + fwd * a_Normal.z);
    }
    else
    {
        localPos = a_Position;
        normal = vec3(0.0, 1.0, 0.0);
        color = a_Normal;
    }

    vec4 worldPos4 = u_Model * vec4(localPos, 1.0);
    v_WorldPos = worldPos4.xyz;
    v_WorldNormal = mat3(transpose(inverse(u_Model))) * normal;
    v_Color = color;
    v_Mode = u_RenderMode;
    gl_Position = u_Projection * u_View * worldPos4;
}
