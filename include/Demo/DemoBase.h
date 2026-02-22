#pragma once

#include <string>

class ICamera;
class ShaderHotReload;

class DemoBase{
public:
    virtual ~DemoBase()=default;

    virtual void OnInit() = 0;
    virtual void OnDestroy() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender(const ICamera &camera) = 0;
    virtual void OnImGui() {}
    virtual void OnResize(int width, int height) {}

    virtual const char* GetName() const = 0;
    virtual const char* GetDescription() const = 0;

    // register shaders
    virtual void RegisterShaders(ShaderHotReload &hotReload) {}
};
