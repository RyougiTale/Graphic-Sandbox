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
        // glBindImageTexture:
        // binding=0, 纹理ID, 不分层, 
        // 绑到 image unit 0（给compute用imageStore写）
        // image unit 编号0, 对应binding=0
        // 绑定的纹理对象m_Texture
        // level=0, mipmap层级, 0=原始分辨率
        // layered=GL_FALSE 不是纹理数组, 不需要按层访问
        // layer=0, 具体那一层(此参数无意义)
        // access=GL_WRITE_ONLY 只写
        // format=GL_RGBA32F 像素格式, 需要和纹理创建时一致
        glBindImageTexture(0, m_Texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        // 向上取整除法: 确保覆盖所有像素
        // 内部调用glDispatchCompute(x, y, 1)跑32x32x16x16次compute shader
        m_ComputeShader.Dispatch((m_TexWidth + 15) / 16, (m_TexHeight + 15) / 16);
        // GPU 开始并行执行 32×32 = 1024 个工作组，共 262144 个线程
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        // 同步
    }

    // Step 2: Display texture on fullscreen quad
    if (m_DisplayShader.IsValid())
    {
        m_DisplayShader.Use();
        // 用纹理单元0
        m_DisplayShader.SetInt("u_Texture", 0);
        // 激活纹理单元0
        glActiveTexture(GL_TEXTURE0);
        // 绑到 texture unit 0（给fragment用texture读）
        glBindTexture(GL_TEXTURE_2D, m_Texture);
        // OpenGL 核心模式（core profile）规定必须有 VAO 绑定才能调用 glDrawArrays
        // 空壳
        glBindVertexArray(m_VAO);
        // 跑3次vertex shader
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

    // 创建纹理对象, 类似 glGenBuffers, 返回一个唯一 ID
    glGenTextures(1, &m_Texture);
    // 绑定到 GL_TEXTURE_2D 目标(状态机模式, 后续操作都针对这张纹理)
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    // 分配显存并指定格式, 但不填数据(nullptr), 留给 compute shader 每帧写入
    // GL_RGBA32F: 内部格式, 每像素 4 通道 × 32 位浮点 = 16 字节
    // GL_RGBA, GL_FLOAT: 描述 nullptr 数据的格式(这里没数据, 但 API 要求填)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    // 采样过滤: 当纹理被缩小(MIN)或放大(MAG)时用线性插值, 而非最近邻
    // GL_LINEAR: 取周围 4 个像素加权平均, 画面更平滑
    // GL_NEAREST: 取最近的 1 个像素, 画面有锯齿但更锐利
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 纹理坐标超出 0~1 范围时的处理方式(S=水平, T=垂直)
    // GL_CLAMP_TO_EDGE: 钳制到边缘像素颜色, 不重复也不出现黑边
    // 其他选项: GL_REPEAT(平铺重复), GL_MIRRORED_REPEAT(镜像重复)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // 解绑, 防止后续误操作修改这张纹理
    glBindTexture(GL_TEXTURE_2D, 0);
}
