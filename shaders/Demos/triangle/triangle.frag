#version 460 core

in vec3 v_Color;

uniform vec3 u_TintColor;

out vec4 FragColor;

void main() {
    FragColor = vec4(v_Color * u_TintColor, 1.0);
}
