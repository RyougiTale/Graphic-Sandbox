#version 460 core

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec3 v_Color;
flat in int v_Mode;

uniform vec3 u_LightDir;
uniform vec3 u_ViewPos;

out vec4 FragColor;

void main()
{
    if (v_Mode == 2)
    {
        FragColor = vec4(v_Color, 1.0);
        return;
    }

    vec3 N = normalize(v_WorldNormal);
    vec3 L = normalize(u_LightDir);
    vec3 V = normalize(u_ViewPos - v_WorldPos);
    vec3 H = normalize(L + V);

    float ambient = 0.16;
    float diffuse = max(dot(N, L), 0.0) * 0.68;
    float specular = pow(max(dot(N, H), 0.0), 36.0) * 0.35;
    float backDiffuse = max(dot(-N, L), 0.0) * 0.18;

    vec3 finalColor = v_Color * (ambient + diffuse + backDiffuse) + vec3(specular);
    FragColor = vec4(finalColor, 1.0);
}
