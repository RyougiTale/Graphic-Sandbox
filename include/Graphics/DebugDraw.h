#pragma once

#include "Graphics/Shader.h"
#include <glad/glad.h>

class ICamera;

class DebugDraw
{
public:
    void Init();
    void Shutdown();
    void Render(const ICamera &camera, float aspectRatio);
    void RenderImGui();

private:
    void BuildAxesGeometry();
    void BuildGridGeometry();
    void RebuildGrid();

    Shader m_Shader;

    GLuint m_AxesVAO = 0, m_AxesVBO = 0;
    GLuint m_GridVAO = 0, m_GridVBO = 0;
    int m_GridVertexCount = 0;

    bool m_ShowAxes = true;
    bool m_ShowGrid = true;
    int m_GridSize = 20;
    float m_GridSpacing = 1.0f;
    float m_GridColor[3] = {0.4f, 0.4f, 0.4f};
};
