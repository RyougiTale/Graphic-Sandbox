#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include "Graphics/ComputeShader.h"
#include <glad/glad.h>

class ComputeDemo : public DemoBase {
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera& camera, float aspectRatio) override;
    void OnImGui() override;
    void OnResize(int width, int height) override;
    void RegisterShaders(ShaderHotReload& hotReload) override;

    const char* GetName() const override { return "Compute Shader"; }
    const char* GetDescription() const override {
        return "GPU compute shader generating animated patterns on a texture.";
    }

private:
    void CreateTexture(int width, int height);

    ComputeShader m_ComputeShader;
    Shader m_DisplayShader;
    GLuint m_Texture = 0;
    GLuint m_VAO = 0;
    int m_TexWidth = 512;
    int m_TexHeight = 512;
    float m_Time = 0.0f;
    float m_Speed = 1.0f;
};
