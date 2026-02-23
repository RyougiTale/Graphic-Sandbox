#include "Demos/ComputeDemo.h"
#include "Camera/ICamera.h"
#include "Graphics/ShaderHotReload.h"
#include <imgui.h>

void ComputeDemo::OnInit()
{
    // compute.comp — compute shader，生成动态波纹图案，写入 texture
    // fullscreen.vert — 全屏三角形，用 gl_VertexID 生成顶点，不需要 VBO
    // fullscreen.frag — 采样 texture 显示到屏幕
    m_ComputeShader.LoadFromFile("shaders/demos/compute/compute.comp");
    m_DisplayShader.LoadFromFiles("shaders/demos/compute/fullscreen.vert",
                                  "shaders/demos/compute/fullscreen.frag");
    CreateTexture(m_TexWidth, m_TexHeight);

    // Empty VAO for fullscreen triangle (vertices generated in shader via gl_VertexID)
    glGenVertexArrays(1, &m_VAO);
}

void ComputeDemo::OnDestroy()
{
    if (m_Texture)
    {
        glDeleteTextures(1, &m_Texture);
        m_Texture = 0;
    }
    if (m_VAO)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

void ComputeDemo::OnUpdate(float deltaTime)
{
    m_Time += deltaTime * m_Speed;
}

// OnRender — compute shader 写 texture → memory barrier → display shader 画全屏三角形
void ComputeDemo::OnRender(const ICamera &, float /*aspectRatio*/)
{
    // Step 1: Compute shader writes to texture
    if (m_ComputeShader.IsValid())
    {
        m_ComputeShader.Use();
        m_ComputeShader.SetFloat("u_Time", m_Time);
        glBindImageTexture(0, m_Texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        m_ComputeShader.Dispatch((m_TexWidth + 15) / 16, (m_TexHeight + 15) / 16);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // Step 2: Display texture on fullscreen quad
    if (m_DisplayShader.IsValid())
    {
        m_DisplayShader.Use();
        m_DisplayShader.SetInt("u_Texture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_Texture);
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }
}

void ComputeDemo::OnImGui()
{
    ImGui::SliderFloat("Speed", &m_Speed, 0.0f, 5.0f);

    if (ImGui::Button("Reset Time"))
    {
        m_Time = 0.0f;
    }

    int texSize[2] = {m_TexWidth, m_TexHeight};
    if (ImGui::InputInt2("Texture Size", texSize))
    {
        texSize[0] = texSize[0] < 1 ? 1 : (texSize[0] > 4096 ? 4096 : texSize[0]);
        texSize[1] = texSize[1] < 1 ? 1 : (texSize[1] > 4096 ? 4096 : texSize[1]);
        if (texSize[0] != m_TexWidth || texSize[1] != m_TexHeight)
        {
            m_TexWidth = texSize[0];
            m_TexHeight = texSize[1];
            CreateTexture(m_TexWidth, m_TexHeight);
        }
    }
}

void ComputeDemo::OnResize(int /*width*/, int /*height*/)
{
    // Could resize texture to match window
}

void ComputeDemo::RegisterShaders(ShaderHotReload &hotReload)
{
    hotReload.Watch(&m_ComputeShader);
    hotReload.Watch(&m_DisplayShader);
}

void ComputeDemo::CreateTexture(int width, int height)
{
    if (m_Texture)
    {
        glDeleteTextures(1, &m_Texture);
    }

    glGenTextures(1, &m_Texture);
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
