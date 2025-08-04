#include "wCorePCH.hpp"
#include "TungstenCore/Application.hpp"

namespace wCore
{
    Application::Application()
    {
    }

    Application::RunOutput Application::Run()
    {
        W_DEBUG_LOG_INFO("Hello, From Application.Run!");

        W_DEBUG_LOG_INFO("Creating Scene...");
        SceneIndex sceneIndex = m_componentSystem.CreateScene();
        W_DEBUG_LOG_INFO("Created Scene At Index: {}", sceneIndex);

        m_componentSystem.CreateComponent(1, sceneIndex);
        m_componentSystem.CreateComponent(2, sceneIndex);

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