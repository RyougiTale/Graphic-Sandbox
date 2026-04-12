#version 460 core

in vec3 v_WorldPos;
in vec3 v_WorldNormal;

uniform vec3 u_LightDir;
uniform vec3 u_ViewPos;
uniform vec3 u_Color;
uniform vec3 u_BackColor;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(v_WorldNormal);
    vec3 L = normalize(u_LightDir);
    vec3 V = normalize(u_ViewPos - v_WorldPos);

    // 双面光照: 正面用 u_Color, 背面用 u_BackColor
    bool frontFace = gl_FrontFacing;
    vec3 baseColor = frontFace ? u_Color : u_BackColor;
    vec3 sN = frontFace ? N : -N;

    // Blinn-Phong
    vec3 H = normalize(L + V);
    float ambient  = 0.15;
    float diffuse  = max(dot(sN, L), 0.0) * 0.7;
    float specular = pow(max(dot(sN, H), 0.0), 32.0) * 0.3;
    float backDiffuse = max(dot(-sN, L), 0.0) * 0.15;

    vec3 finalColor = baseColor * (ambient + diffuse + backDiffuse) + vec3(specular);
    FragColor = vec4(finalColor, 1.0);
}
