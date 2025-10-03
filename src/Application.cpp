#include "wCorePCH.hpp"
#include "TungstenCore/Application.hpp"

namespace wCore
{
    Application::Application()
        : m_componentSystem(*this)
    {
    }

    Application::RunOutput Application::Run()
    {
        W_DEBUG_LOG_INFO("Hello, From Application.Run!");

        W_DEBUG_LOG_INFO("Creating Scene...");
        SceneIndex sceneIndex = m_componentSystem.CreateScene();
        W_DEBUG_LOG_INFO("Created Scene At Index: {}", sceneIndex);
        SceneIndex sceneIndex1 = m_componentSystem.CreateScene();
        W_DEBUG_LOG_INFO("Created Scene1 At Index: {}", sceneIndex1);

        std::vector<ComponentIndex> indexes;
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(2, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(2, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(2, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(2, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(2, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(2, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(1, sceneIndex));
        indexes.emplace_back(m_componentSystem.CreateComponent(2, sceneIndex));
        W_DEBUG_LOG_INFO("All Created");

        return Application::RunOutput(0);
    }
/*
    uint32_t Application::CreateScene()
    {
        m_scenes.push_back(Scene());
        return m_scenes.size();
    }

    void Application::DestroyScene(uint32_t sceneIndex)
    {
        m_scenes.erase(m_scenes.begin() + sceneIndex);
    }*/
}