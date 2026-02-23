#pragma once

#include "Demo/DemoBase.h"
#include "Graphics/Shader.h"
#include <glad/glad.h>

class TriangleDemo : public DemoBase
{
public:
    void OnInit() override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(const ICamera &camera, float aspectRatio) override;
    void OnImGui() override;
    void RegisterShaders(ShaderHotReload &hotReload) override;

    const char *GetName() const override { return "Triangle"; }
    const char *GetDescription() const override
    {
        return "Basic rotating triangle demo. Tests shader hot-reload functionality.";
    }

private:
    Shader m_Shader;
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;

    float m_RotationSpeed = 1.0f;
    float m_Rotation = 0.0f;
    float m_Scale = 1.0f;
    float m_Color[3] = {1.0f, 0.5f, 0.2f};
};
