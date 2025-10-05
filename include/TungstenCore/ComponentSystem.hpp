#ifndef TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP
#define TUNGSTEN_CORE_COMPONENT_SYSTEM_HPP

#include "TungstenCore/ComponentSetup.hpp"
#include <span>

namespace wCore
{
    using SceneIndex = wIndex;
    inline constexpr SceneIndex InvalidScene = 0;
    inline constexpr SceneIndex SceneIndexStart = 1;

    class SceneGeneration
    {
    public:
        constexpr SceneGeneration() noexcept
            : generation(0) {}

        friend constexpr bool operator==(const SceneGeneration&, const SceneGeneration&) = default;

    private:
        uint32_t generation;
        friend class ComponentSystem;
    };

    struct SceneHandle
    {
        SceneHandle(SceneIndex a_sceneIndex, SceneGeneration a_generation)
            : sceneIndex(a_sceneIndex), generation(a_generation) {}

        SceneIndex sceneIndex;
        SceneGeneration generation;
    };

    using ComponentIndex = wIndex;
    inline constexpr ComponentIndex InvalidComponent = 0;
    inline constexpr ComponentIndex ComponentIndexStart = 1;

    class ComponentGeneration
    {
        uint32_t generation;
        friend class ComponentSystem;
    };

    template<typename T>
    struct ComponentHandle
    {
        SceneHandle sceneHandle;
        ComponentIndex componentIndex;
        ComponentGeneration generation;
    };

    class ComponentSystem
    {
    public:
        ComponentSystem(Application& app) noexcept;
        ~ComponentSystem() noexcept;

        ComponentSystem(const ComponentSetup&) = delete;
        ComponentSystem& operator=(const ComponentSetup&) = delete;

        // Scenes
        void ReserveScenes(wIndex minCapacity);
        inline void ReserveSceneFreeList(wIndex minCapacity) { m_sceneFreeList.Reserve(minCapacity); }

        [[nodiscard]] inline SceneHandle CreateScene() { return CreateScene(""); }
        [[nodiscard]] SceneHandle CreateScene(std::string_view name);
        void DestroyScene(SceneIndex sceneIndex) noexcept;
        [[nodiscard]] inline bool SceneExists(SceneHandle sceneHandle) const noexcept { return sceneHandle.generation == m_sceneGenerations[sceneHandle.sceneIndex]; };

        //inline const Scene& GetScene(uint32_t sceneIndex) const { return m_scenes[sceneIndex - 1]; }
/*
        [[nodiscard]] inline const std::string& GetSceneName(SceneIndex sceneIndex) const noexcept { return m_sceneData[sceneIndex - 1].name; }
        inline void SetSceneName(SceneIndex sceneIndex, std::string_view name) { m_sceneData[sceneIndex - 1].name = name; }
        inline void SetSceneName(SceneIndex sceneIndex, std::string&& name) noexcept { m_sceneData[sceneIndex - 1].name = std::move(name); }

        [[nodiscard]] inline wIndex GetSceneCount() const noexcept { return m_sceneData.size() - m_sceneFreeList.size(); }
        [[nodiscard]] inline wIndex GetSceneSlotCount() const noexcept { return m_sceneData.size(); }
        [[nodiscard]] inline wIndex GetSceneSlotCapacity() const noexcept { return m_sceneData.capacity(); }
        */
        [[nodiscard]] inline wIndex GetSceneFreeListCount() const noexcept { return m_sceneFreeList.Count(); }
        [[nodiscard]] inline wIndex GetSceneFreeListCapacity() const noexcept { return m_sceneFreeList.Capacity(); }

        // Components
        //inline void ReserveComponents(wIndex sceneIndex, wIndex minCapacity) { ReserveKnownList<Component>(m_componentLists[GetSceneStartListIndex(sceneIndex) + ComponentListOffset], minCapacity); }

        template<typename T>
        inline void ReserveComponents(SceneIndex sceneIndex, wIndex minCapacity) { ReserveKnownList<T>(GetComponentListHeader<T>(sceneIndex), minCapacity); }
/*
        template<typename T>
        [[nodiscard]] ComponentIndex CreateComponent(wIndex sceneIndex)
        {
            const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
            const wIndex listIndex = ComponentSetup::CreateComponent<T>(m_componentLists[sceneStartListIndex + GetComponentListOffset<T>()], m_app);
            return ComponentSetup::EmplaceKnownList<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset], 0, listIndex) + 1;
        }

        template<typename T>
        [[nodiscard]] T& GetComponent(wIndex sceneIndex, ComponentIndex componentIndex)
        {
            const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
            const wIndex listIndex = IndexKnownList<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset], componentIndex - 1);
            return IndexKnownList<T>(m_componentLists[sceneStartListIndex + GetComponentListOffset<T>()], listIndex);
        }

        template<typename T>
        [[nodiscard]] const T& GetComponent(wIndex sceneIndex, ComponentIndex componentIndex) const
        {
            const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
            const wIndex listIndex = IndexKnownList<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset], componentIndex - 1);
            return IndexKnownList<T>(m_componentLists[sceneStartListIndex + GetComponentListOffset<T>()], listIndex);
        }

        [[nodiscard]] inline wIndex GetComponentCount(uint32_t sceneIndex) const { return GetKnownListCount<Component>(m_componentLists[GetSceneStartListIndex(sceneIndex) + ComponentListOffset]); }
        [[nodiscard]] inline wIndex GetComponentCapacity(uint32_t sceneIndex) const { return GetKnownListCapacity<Component>(m_componentLists[GetSceneStartListIndex(sceneIndex) + ComponentListOffset]); }

        [[nodiscard]] inline wIndex GetComponentNameCount(uint32_t sceneIndex) const { return GetKnownListCount<std::string>(m_componentLists[GetSceneStartListIndex(sceneIndex) + ComponentListOffset]); }
        [[nodiscard]] inline wIndex GetComponentNameCapacity(uint32_t sceneIndex) const { return GetKnownListCapacity<std::string>(m_componentLists[GetSceneStartListIndex(sceneIndex) + ComponentListOffset]); }*/

        template<typename T>
        [[nodiscard]] inline wIndex GetComponentCount(wIndex sceneIndex) const { return GetKnownListCount<T>(GetComponentListHeader<T>(sceneIndex)); }

        template<typename T>
        [[nodiscard]] inline wIndex GetComponentCapacity(wIndex sceneIndex) const { return GetKnownListCapacity<T>(GetComponentListHeader<T>(sceneIndex)); }

        // API
        [[nodiscard]] inline ComponentSetup& GetComponentSetup() { return m_componentSetup; };
        [[nodiscard]] inline const ComponentSetup& GetComponentSetup() const { return m_componentSetup; }

        // Internal
        void ReserveComponents(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex minCapacity);
        [[nodiscard]] ComponentIndex CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex);

        [[nodiscard]] inline wIndex GetComponentCount(ComponentTypeIndex componentTypeIndex, wIndex sceneIndex) const;
        [[nodiscard]] inline wIndex GetComponentCapacity(ComponentTypeIndex componentTypeIndex, wIndex sceneIndex) const;

    private:
        static constexpr wIndex InitialCapacity = 8;
        static inline constexpr wIndex CalculateNextCapacity(wIndex current) noexcept
        {
            if (current)
            {
                return current * 2;
            }
            return InitialCapacity;
        }

        struct ListHeader
        {
            void* data;
            wIndex count;
            wIndex capacity;
        };

        struct SceneData
        {
            uint32_t nameIndex;
        };
/*
        template<typename T>
        static void ReserveKnownList(ComponentSetup::ListHeader& header, wIndex num)
        {
            if (num > header.Capacity<T>() - header.Begin<T>())
            {
                ComponentSetup::ReallocateKnownList<T>(header, num);
            }
        }

        template<typename T>
        static wIndex AddToKnownList(ComponentSetup::ListHeader& header, T& value)
        {
            T* end = header.End<T>();
            const T* const begin = header.Data<T>();
            const T* const capacity = header.Capacity<T>();

            if (header.count == header.capacity)
            {
                if (&value >= data && &value < data + header.count)
                {
                    T tmp = value;
                    ReallocateKnownList<T>(header, CalculateNextCapacity(header.count + 1, header.capacity));
                    end = header.End<T>();
                    std::construct_at(end, std::move_if_noexcept(tmp));
                    header.end = end + 1;
                    return oldCount;
                }

                ReallocateKnownList<T>(header, CalculateNextCapacity(header.count + 1, header.capacity));
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
        static void DestroyKnownList(ComponentSetup::ListHeader& header) noexcept(std::is_nothrow_destructible_v<T>) noexcept
        {
            if (!ComponentSetup::IsSentinel<T>(header.begin))
            {
                ComponentSetup::DestroyKnownListUnchecked<T>(header);
            }
        }

        template<typename T>
        [[nodiscard]] inline static wIndex GetKnownListCount(ComponentSetup::ListHeader& header) noexcept { return header.End<T>() - header.Begin<T>(); }

        template<typename T>
        [[nodiscard]] inline static wIndex GetKnownListCapacity(ComponentSetup::ListHeader& header) noexcept { return header.Capacity<T>() - header.Begin<T>(); }

        template<typename T>
        [[nodiscard]] inline static T& IndexKnownList(ComponentSetup::ListHeader& header, wIndex index) noexcept { return header.Begin<T>()[index]; }

        template<typename T>
        [[nodiscard]] inline static const T& IndexKnownList(const ComponentSetup::ListHeader& header, wIndex index) noexcept { return header.Begin<T>()[index]; }

        template<typename T>
        [[nodiscard]] inline static T& KnownListAt(ComponentSetup::ListHeader& header, wIndex index) { W_ASSERT(index < GetKnownListCount<T>(header), "KnownList Index {} out of Range! KnownList Count: {}", index, GetKnownListCount<T>(header)); return Index<T>(header, index); }

        template<typename T>
        [[nodiscard]] inline static const T& KnownListAt(const ComponentSetup::ListHeader& header, wIndex index) { W_ASSERT(index < GetKnownListCount<T>(header), "KnownList Index {} out of Range! KnownList Count: {}", index, GetKnownListCount<T>(header)); return Index<T>(header, index); }

        template<typename T>
        [[nodiscard]] inline static std::span<T> GetSpanFromKnownList(ComponentSetup::ListHeader& header) noexcept { return { header.Begin<T>(), header.End<T>() }; }

        template<typename T>
        [[nodiscard]] inline static std::span<const T> GetSpanFromKnownList(const ComponentSetup::ListHeader& header) noexcept { return { header.Begin<T>(), header.End<T>() }; }
*/
        void ReallocateScenes(wIndex newCapacity);
        void DeleteSceneContent(std::size_t sceneStartPtrIndex);

        void DestroyComponentList(std::size_t sceneStartPtrIndex, ComponentTypeIndex componentTypeIndex);

        // Component
        // std::string names
        [[nodiscard]] inline std::size_t GetSceneStartListIndex(SceneIndex sceneIndex) const noexcept { return (sceneIndex - 1) * m_currentComponentTypeCount; }

        [[nodiscard]] static constexpr std::size_t GetComponentListOffset(ComponentTypeIndex componentTypeIndex) noexcept { return componentTypeIndex - 1; }
        [[nodiscard]] inline std::size_t GetComponentListIndex(SceneIndex sceneIndex, ComponentTypeIndex componentTypeIndex) const noexcept { return GetSceneStartListIndex(sceneIndex) + GetComponentListOffset(componentTypeIndex); }
        //[[nodiscard]] inline ComponentSetup::ComponentListHeader& GetComponentListHeader(SceneIndex sceneIndex, ComponentTypeIndex componentTypeIndex) noexcept { return m_componentLists[GetComponentListIndex(sceneIndex, componentTypeIndex)]; }
        //[[nodiscard]] inline const ComponentSetup::ComponentListHeader& GetComponentListHeader(SceneIndex sceneIndex, ComponentTypeIndex componentTypeIndex) const noexcept { return m_componentLists[GetComponentListIndex(sceneIndex, componentTypeIndex)]; }

        template<typename T>
        [[nodiscard]] inline std::size_t GetComponentListOffset() const noexcept { return GetComponentListOffset(m_componentSetup.GetComponentTypeIndex<T>()); }

        template<typename T>
        [[nodiscard]] inline std::size_t GetComponentListIndex(SceneIndex sceneIndex) const noexcept { return GetSceneStartListIndex(sceneIndex) + GetComponentListOffset<T>(); }
/*
        template<typename T>
        [[nodiscard]] inline ComponentSetup::ListHeader& GetComponentListHeader(SceneIndex sceneIndex) noexcept { return m_componentLists[GetComponentListIndex<T>(sceneIndex)]; }

        template<typename T>
        [[nodiscard]] inline const ComponentSetup::ListHeader& GetComponentListHeader(SceneIndex sceneIndex) const noexcept { return m_componentLists[GetComponentListIndex<T>(sceneIndex)]; }
*/

        // void* data and Generation* for each component type
        static inline constexpr std::size_t GetPointerCountPerScene(std::size_t componentTypeCount) noexcept { return 2 * componentTypeCount; }

        // wIndex slotCount and wIndex capacity for each component type
        static inline constexpr std::size_t GetIndexCountPerScene(std::size_t componentTypeCount) noexcept { return 2 * componentTypeCount; }

        Application& m_app;
        ComponentSetup m_componentSetup;
        wIndex m_currentComponentTypeCount;

        std::byte* m_scenes;
        void** m_ptrs;
        SceneGeneration* m_sceneGenerations;
        wIndex* m_indexes;
        wUtils::RelocatableFreeListHeader<ComponentIndex>* m_freeLists;
        SceneData* m_sceneData;
        wIndex m_sceneSlotCount;
        wIndex m_sceneSlotCapacity;

        wUtils::FreeList<SceneIndex> m_sceneFreeList;
        wUtils::SlotList<std::string> m_sceneNames;
    };

    class Scene
    {
    public:
        Scene(SceneHandle handle, ComponentSystem& componentSystem)
            : m_handle(handle), m_componentSystem(componentSystem)
        {
        }

        [[nodiscard]] constexpr SceneHandle GetHandle() const { return m_handle; }
        [[nodiscard]] constexpr SceneIndex GetIndex()  const { return m_handle.sceneIndex; }
        [[nodiscard]] constexpr SceneGeneration GetGeneration()  const { return m_handle.generation; }
        [[nodiscard]] constexpr ComponentSystem& GetComponentSystem()  const { return m_componentSystem; }

    private:
        SceneHandle m_handle;
        ComponentSystem& m_componentSystem;
    };

    struct Component
    {
    public:
        Component(ComponentHandle handle, ComponentSystem& componentSystem)
            : m_handle(handle), m_componentSystem(componentSystem)
        {
        }

        [[nodiscard]] constexpr Scene GetScene() const { return Scene(m_handle.sceneHandle, m_componentSystem); }
        [[nodiscard]] constexpr ComponentHandle GetHandle() const { return m_handle; }
        [[nodiscard]] constexpr SceneHandle GetSceneHandle() const { return m_handle.sceneHandle; }
        [[nodiscard]] constexpr SceneIndex GetSceneIndex() const { return m_handle.sceneHandle.sceneIndex; }
        [[nodiscard]] constexpr SceneGeneration GetSceneGeneration() const { return m_handle.sceneHandle.generation; }
        [[nodiscard]] constexpr ComponentIndex GetIndex() const { return m_handle.componentIndex; }
        [[nodiscard]] constexpr ComponentGeneration GetGeneration() const { return m_handle.generation; }
        [[nodiscard]] constexpr ComponentSystem& GetComponentSystem() const { return m_componentSystem; }

    private:
        ComponentHandle m_handle;
        ComponentSystem& m_componentSystem;
    };
}

#endif