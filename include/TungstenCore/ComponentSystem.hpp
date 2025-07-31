#ifndef TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP
#define TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP

#include "TungstenCore/ComponentSetup.hpp"
#include "TungstenCore/Scene.hpp"

namespace wCore
{
    class ComponentSystem
    {
    public:
        uint32_t CreateScene();
        void DestroyScene(uint32_t sceneIndex);
        //inline const Scene& GetScene(uint32_t sceneIndex) const { return m_scenes[sceneIndex - 1]; }
        //inline uint32_t GetComponentCount(uint32_t sceneIndex) const { return m_scenes.size(); }

        template<typename T>
        uint32_t CreateComponent(uint32_t sceneIndex, std::size_t* componentDataSize, uint8_t* componentData);
        uint32_t GetComponentType(uint32_t sceneIndex, uint32_t componentIndex);
        uint32_t GetComponentChildCount(uint32_t sceneIndex, uint32_t componentIndex);

        inline ComponentSetup& GetComponentSetup() { return m_componentSetup; };

    private:
        ComponentSetup m_componentSetup;
    };
}

#endif