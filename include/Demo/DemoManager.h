#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "Demo/DemoBase.h"

class ICamera;
class ShaderHotReload;

class DemoManager
{
public:
    ~DemoManager();
    using DemoFactory = std::function<std::unique_ptr<DemoBase>()>;

    void RegisterDemo(const std::string &name, DemoFactory factory);

    template <typename T>
    void Register()
    {
        RegisterDemo(T().GetName(), []()
                     { return std::make_unique<T>(); });
    }
    void Init(ShaderHotReload &hotReload);
    void Shutdown();

    void SwitchTo(const std::string &name);
    void SwitchTo(int index);

    void Update(float deltaTime);
    void Render(const ICamera &camera, float aspectRatio);
    void RenderImGui();
    void OnResize(int width, int height);

    DemoBase *GetCurrentDemo() const { return m_CurrentDemo.get(); }
    const std::vector<std::string> &GetDemoNames() const { return m_DemoNames; }
    int GetCurrentIndex() const { return m_CurrentIndex; }

private:
    std::vector<std::string> m_DemoNames;
    std::vector<DemoFactory> m_DemoFactories;

    std::unique_ptr<DemoBase> m_CurrentDemo;
    int m_CurrentIndex = -1;
    ShaderHotReload *m_HotReload = nullptr;
};