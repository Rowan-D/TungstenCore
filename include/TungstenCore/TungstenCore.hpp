#ifndef TUNGSTEN_CORE_TUNGSTEN_CORE_HPP
#define TUNGSTEN_CORE_TUNGSTEN_CORE_HPP

#include <vector>
#include "TungstenUtils/TungstenUtils.hpp"
#include "Scene.hpp"
#include "ComponentManager.hpp"

namespace wCore
{
    class Application
    {
    public:
        struct RunOutput
        {
            RunOutput(int a_exitCode)
                : exitCode(a_exitCode)
            {
            }

            int exitCode;
        };

        Application();
        RunOutput Run();

        uint32_t CreateScene();
        void DestroyScene(uint32_t sceneIndex);
        inline const Scene& GetScene(uint32_t sceneIndex) const { return m_scenes[sceneIndex - 1]; }
        inline uint32_t GetComponentCount(uint32_t sceneIndex) const { return m_scenes.size(); }

        template<typename T>
        uint32_t CreateComponent(uint32_t sceneIndex, std::size_t* componentDataSize, uint8_t* componentData);
        uint32_t GetComponentType(uint32_t sceneIndex, uint32_t componentIndex);
        uint32_t GetComponentChildCount(uint32_t sceneIndex, uint32_t componentIndex);

        inline const ComponentManager& GetComponentManager() const { return m_componentManager; }

    private:
        ComponentManager m_componentManager;
        std::vector<Scene> m_scenes;
    };
}

#endif