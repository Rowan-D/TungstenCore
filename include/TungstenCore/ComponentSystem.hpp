#ifndef TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP
#define TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP

#include "TungstenCore/ComponentSetup.hpp"
#include "TungstenCore/Scene.hpp"

namespace wCore
{
    using SceneIndex = wIndex;
    using ComponentIndex = wIndex;

    class ComponentSystem
    {
    public:
        ComponentSystem();

        ComponentSystem(const ComponentSetup&) = delete;
        ComponentSystem& operator=(const ComponentSetup&) = delete;

        void ReserveScenes(wIndex sceneCapacity);
        SceneIndex CreateScene();
        void DestroyScene(SceneIndex sceneIndex);
        bool SceneExists(SceneIndex sceneIndex);
        //inline const Scene& GetScene(uint32_t sceneIndex) const { return m_scenes[sceneIndex - 1]; }
        //inline uint32_t GetComponentCount(uint32_t sceneIndex) const { return m_scenes.size(); }

        inline wIndex SceneCount() const noexcept { return m_sceneData.size(); }

        void ReserveComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex componentCapacity);
        ComponentIndex CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex);

        template<typename T>
        void ReserveComponent(wIndex sceneIndex, wIndex componentCapacity)
        {
        }

        /*template<typename T>
        ComponentIndex CreateComponent(wIndex sceneIndex, T& component)
        {
            const std::size_t sceneSizePointers = GetSceneSizePointers();
            const wIndex componentID = m_componentSetup.GetComponentTypeIndex<T>();
            m_componentLists[sceneIndex * sceneSizePointers + (componentID + 1) * 3];
        }

        template<typename T>
        T& GetComponent(wIndex sceneIndex, wIndex componentIndex);

        template<typename T>
        T& GetComponentCount(wIndex sceneIndex, wIndex componentIndex);
        template<typename T>
        T& GetComponentCapacity(wIndex sceneIndex, wIndex componentIndex);*/

        inline ComponentSetup& GetComponentSetup() { return m_componentSetup; };

    private:
        static constexpr wIndex InitialCapacity = 8;
        static constexpr std::size_t BeginPtrOffset = 0;
        static constexpr std::size_t EndPtrOffset = 1;
        static constexpr std::size_t CapacityPtrOffset = 2;

        struct Component
        {
            wIndex nameIndex;
            wIndex listIndex;
        };

        struct SceneData
        {
            SceneData() : name() {}

            std::string name; // to make an option
        };

        void ReallocateScenes(wIndex sceneCapacity);

        void ReallocateComponentList(SceneIndex sceneIndex, ComponentTypeIndex componentTypeIndex, wIndex newCapacity);

        static inline constexpr wIndex CalculateNextCapacity(wIndex requested, wIndex current) noexcept
        {
            if (!current)
            {
                return std::max(InitialCapacity, requested);
            }
            return std::max(current * 2, requested);
        }

        // Component
        // std::string names
        inline std::size_t GetSceneSizePointers() const noexcept { return 3 * (2 * m_componentSetup.GetComponentTypeCount()); }
        inline std::size_t GetSceneSizeBytes() const noexcept { return GetSceneSizePointers() * sizeof(void*); }
        inline std::size_t GetSceneStartPtrIndex(SceneIndex sceneIndex) const noexcept { return sceneIndex * GetSceneSizePointers(); }

        inline std::size_t GetComponentPointerOffset(ComponentTypeIndex componentTypeIndex) const noexcept { return (componentTypeIndex + 1) * 3; }
        inline std::size_t GetComponentStartPtrIndex(SceneIndex sceneIndex, ComponentTypeIndex componentTypeIndex) const noexcept { return GetSceneStartPtrIndex(sceneIndex) + GetComponentPointerOffset(componentTypeIndex); }

        //inline std::size_t GetComponentListStartPtrIndex(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex) const noexcept;

        ComponentSetup m_componentSetup;

        void** m_componentLists;
        wIndex m_componentListsCapacity;

        std::vector<SceneData> m_sceneData;
        std::vector<wIndex> m_sceneFreeList;
    };
}

#endif