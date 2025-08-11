#ifndef TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP
#define TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP

#include "TungstenCore/ComponentSetup.hpp"
#include "TungstenCore/Scene.hpp"

namespace wCore
{
    using SceneIndex = wIndex;
    inline constexpr SceneIndex InvalidScene = 0;
    inline constexpr SceneIndex SceneIndexStart = 1;

    using ComponentIndex = wIndex;
    inline constexpr ComponentIndex InvalidComponent = 0;
    inline constexpr ComponentIndex ComponentIndexStart = 1;

    class ComponentSystem
    {
    public:
        ComponentSystem(Application& app);
        ~ComponentSystem();

        ComponentSystem(const ComponentSetup&) = delete;
        ComponentSystem& operator=(const ComponentSetup&) = delete;

        void ReserveScenes(wIndex sceneCapacity);
        [[nodiscard]] SceneIndex CreateScene();
        void DestroyScene(SceneIndex sceneIndex);
        inline bool SceneExists(SceneIndex sceneIndex) { return m_componentLists[GetSceneStartListIndex(sceneIndex)].begin; }
        //inline const Scene& GetScene(uint32_t sceneIndex) const { return m_scenes[sceneIndex - 1]; }
        wIndex GetComponentCount(uint32_t sceneIndex) const;

        // TODO:
        inline wIndex SceneCount() const noexcept { return m_sceneData.size(); }

        void ReserveComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex componentCapacity);
        ComponentIndex CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex);

        /*template<typename T>
        void ReserveComponent(wIndex sceneIndex, wIndex componentCapacity)
        {
            KnownListReserve<T>(m_componentSetup.GetComponentTypeIndex<T>());
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
        inline const ComponentSetup& GetComponentSetup() const { return m_componentSetup; }

    private:
        static constexpr std::size_t ComponentListOffset = 0;
        static constexpr std::size_t ComponentNameListOffset = 1;
        static constexpr std::size_t KnownListCount = 2;

        struct Component
        {
            Component(wIndex a_nameIndex, wIndex a_listIndex) : nameIndex(a_nameIndex), listIndex(a_listIndex) {}

            wIndex nameIndex;
            wIndex listIndex;
        };

        struct SceneData
        {
            wIndex nameIndex;
        };

        template<typename T>
        static void ReserveKnownList(ComponentSetup::ListHeader& header, wIndex num)
        {
            if (num > header.Capacity<T>() - header.Begin<T>())
            {
                ReallocateKnownList<T>(header, num);
            }
        }

        template<typename T>
        static wIndex AddToKnownList(ComponentSetup::ListHeader& header, T& value)
        {
            T* end = header.End<T>();
            const T* const begin = header.Begin<T>();
            const T* const capacity = header.Capacity<T>();
            const wIndex oldCount = end - begin;

            if (end == capacity)
            {
                if (&value >= begin && &value < end)
                {
                    T tmp = value;
                    ReallocateKnownList<T>(header, CalculateNextCapacity(oldCount + 1, capacity - begin));
                    end = header.End<T>();
                    std::construct_at(end, std::move_if_noexcept(tmp));
                    header.end = end + 1;
                    return oldCount;
                }

                ReallocateKnownList<T>(header, CalculateNextCapacity(oldCount + 1, capacity - begin));
                end = header.End<T>();
            }

            std::construct_at(end, value);

            header.end = end + 1;
            return oldCount;
        }

        template<typename T>
        static wIndex AddToKnownList(ComponentSetup::ListHeader& header, T&& value)
        {
            T* end = header.End<T>();
            const T* const begin = header.Begin<T>();
            const T* const capacity = header.Capacity<T>();
            const wIndex oldCount = end - begin;

            if (end == capacity)
            {
                if (&value >= begin && &value < end)
                {
                    T tmp = std::move(value);
                    ReallocateKnownList<T>(header, CalculateNextCapacity(oldCount + 1, capacity - begin));
                    end = header.End<T>();
                    std::construct_at(end, std::move_if_noexcept(tmp));
                    header.end = end + 1;
                    return oldCount;
                }

                ReallocateKnownList<T>(header, CalculateNextCapacity(oldCount + 1, capacity - begin));
                end = header.End<T>();
            }

            std::construct_at(end, std::move_if_noexcept(value));

            header.end = end + 1;
            return oldCount;
        }

        template<typename T>
        static void DestroyKnownList(ComponentSetup::ListHeader& header) noexcept
        {
            if (!ComponentSetup::IsSentinel<T>(header.begin))
            {
                ComponentSetup::DestroyKnownListUnchecked<T>(header);
            }
        }

        template<typename T>
        static wIndex GetKnownListCount(ComponentSetup::ListHeader& header) noexcept
        {
            return header.End<T>() - header.Begin<T>();
        }

        void ReallocateScenes(wIndex sceneCapacity);
        void DeleteSceneContent(std::size_t sceneStartPtrIndex);

        void DestroyComponentList(std::size_t sceneStartPtrIndex, ComponentTypeIndex componentTypeIndex);


        // Component
        // std::string names
        inline std::size_t GetSceneSizeLists() const noexcept { return 2 + m_currentComponentTypeCount; }
        inline std::size_t GetSceneSizeBytes() const noexcept { return GetSceneSizeLists() * sizeof(ComponentSetup::ListHeader); }
        inline std::size_t GetSceneStartListIndex(SceneIndex sceneIndex) const noexcept { return (sceneIndex - 1) * GetSceneSizeLists(); }

        inline std::size_t GetComponentListOffset(ComponentTypeIndex componentTypeIndex) const noexcept { return componentTypeIndex + 1; }
        inline std::size_t GetComponentStartListIndex(SceneIndex sceneIndex, ComponentTypeIndex componentTypeIndex) const noexcept { return GetSceneStartListIndex(sceneIndex) + GetComponentListOffset(componentTypeIndex); }

        //inline std::size_t GetComponentListStartPtrIndex(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex) const noexcept;

        Application& m_app;
        ComponentSetup m_componentSetup;
        wIndex m_currentComponentTypeCount;

        ComponentSetup::ListHeader* m_emptyComponentLists;
        ComponentSetup::ListHeader* m_componentLists;
        wIndex m_componentListsCapacity;

        std::vector<SceneData> m_sceneData;
        std::vector<wIndex> m_sceneFreeList;
    };
}

#endif