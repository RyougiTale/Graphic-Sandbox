#version 460 core

out vec2 v_UV;

void main()
{
    vec2 pos[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    v_UV = pos[gl_VertexID] * 0.5 + 0.5;
}
