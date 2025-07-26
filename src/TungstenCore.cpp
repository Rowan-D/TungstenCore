#include "TungstenCore/TungstenCore.hpp"

namespace wCore
{
    Application::Application()
    {
    }

    Application::RunOutput Application::Run()
    {
        return Application::RunOutput(0);
    }

    uint32_t Application::CreateScene()
    {
        m_scenes.push_back(Scene());
        return m_scenes.size();
    }

    void Application::DestroyScene(uint32_t sceneIndex)
    {
        m_scenes.erase(m_scenes.begin() + sceneIndex);
    }
}