#include "Demo/DemoManager.h"
#include "Graphics/ShaderHotReload.h"
#include <imgui.h>
#include <iostream>
#include "Util/GLog.h"

DemoManager::~DemoManager()
{
    Shutdown();
}

void DemoManager::RegisterDemo(const std::string &name, DemoFactory factory)
{
    m_DemoNames.push_back(name);
    m_DemoFactories.push_back(std::move(factory));
}

void DemoManager::Init(ShaderHotReload &hotReload)
{
    LOG_INFO("[DemoManager] Init begin");
    m_HotReload = &hotReload;

    if (!m_DemoFactories.empty())
    {
        SwitchTo(0);
    }
    LOG_INFO("[DemoManager] Init end");
}

void DemoManager::Shutdown()
{
    if (m_CurrentDemo)
    {
        m_CurrentDemo->OnDestroy();
        m_CurrentDemo.reset();
    }
}

void DemoManager::SwitchTo(const std::string &name)
{
    for (size_t i = 0; i < m_DemoNames.size(); ++i)
    {
        if (m_DemoNames[i] == name)
        {
            SwitchTo(static_cast<int>(i));
            return;
        }
    }
    LOG_ERROR("[DemoManager] Demo not found: ", name);
}

void DemoManager::SwitchTo(int index)
{
    LOG_INFO("[DemoManager] Switch To ", index);
    if (index < 0 || index >= static_cast<int>(m_DemoFactories.size()))
    {
        return;
    }
    
    if (index == m_CurrentIndex)
    {
        return;
    }
    
    // 销毁
    if (m_CurrentDemo)
    {
        m_CurrentDemo->OnDestroy();
        m_CurrentDemo.reset();
    }

    // create
    m_CurrentIndex = index;
    m_CurrentDemo = m_DemoFactories[index]();
    LOG_INFO("[DemoManager] Switching to: ", m_CurrentDemo->GetName());
    // init
    m_CurrentDemo->OnInit();

    if (m_HotReload)
    {
        m_CurrentDemo->RegisterShaders(*m_HotReload);
    }
}

void DemoManager::Update(float deltaTime)
{
    if (m_CurrentDemo)
    {
        m_CurrentDemo->OnUpdate(deltaTime);
    }
}

void DemoManager::Render(const ICamera &camera)
{
    if (m_CurrentDemo)
    {
        m_CurrentDemo->OnRender(camera);
    }
}

void DemoManager::RenderImGui()
{
    ImGui::SetNextWindowSize(ImVec2(800, 350), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 760), ImGuiCond_FirstUseEver);
    ImGui::Begin("Demos");

    // Demo selector
    if (ImGui::BeginCombo("Select Demo", m_CurrentIndex >= 0 ? m_DemoNames[m_CurrentIndex].c_str() : "None"))
    {
        for (int i = 0; i < static_cast<int>(m_DemoNames.size()); ++i)
        {
            bool isSelected = (m_CurrentIndex == i);
            if (ImGui::Selectable(m_DemoNames[i].c_str(), isSelected))
            {
                SwitchTo(i);
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Description
    if (m_CurrentDemo)
    {
        const char *desc = m_CurrentDemo->GetDescription();
        if (desc && desc[0])
        {
            ImGui::TextWrapped("%s", desc);
        }
    }

    ImGui::Separator();

    // ImGui
    if (m_CurrentDemo)
    {
        m_CurrentDemo->OnImGui();
    }

    ImGui::End();
}

void DemoManager::OnResize(int width, int height)
{
    if (m_CurrentDemo)
    {
        m_CurrentDemo->OnResize(width, height);
    }
}
