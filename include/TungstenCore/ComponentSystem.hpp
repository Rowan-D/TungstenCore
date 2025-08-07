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
        ComponentSystem();
        ~ComponentSystem();

        ComponentSystem(const ComponentSetup&) = delete;
        ComponentSystem& operator=(const ComponentSetup&) = delete;

        void ReserveScenes(wIndex sceneCapacity);
        SceneIndex CreateScene();
        void DestroyScene(SceneIndex sceneIndex);
        bool SceneExists(SceneIndex sceneIndex);
        //inline const Scene& GetScene(uint32_t sceneIndex) const { return m_scenes[sceneIndex - 1]; }
        wIndex GetComponentCount(uint32_t sceneIndex) const;

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
        inline const ComponentSetup& GetComponentSetup() const { return m_componentSetup; }

    private:
        static constexpr wIndex InitialCapacity = 8;

        static constexpr std::size_t BeginPtrOffset = 0;
        static constexpr std::size_t EndPtrOffset = 1;
        static constexpr std::size_t CapacityPtrOffset = 2;

        static constexpr std::size_t ComponentListPtrOffset = 0;
        static constexpr std::size_t ComponentNameListPtrOffset = 3;

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
        void KnownListReserve(void** listStart, wIndex num)
        {
            const void*& capacityVoid = listStart[CapacityPtrOffset];
            if (capacityVoid)
            {
                const T* const capacity = static_cast<T*>(capacityVoid);
                const T* const begin = static_cast<T*>(listStart[BeginPtrOffset]);
                if (num > capacity - begin)
                {
                    ReallocateKnownList<T>(listStart, num);
                }
            }
        }

        template<typename T>
        wIndex KnownListAdd(void** listStart, T& value)
        {
            void*& endVoid = listStart[EndPtrOffset];
            T* end = static_cast<T*>(endVoid);
            const T* const begin = static_cast<T*>(listStart[BeginPtrOffset]);
            const T* const capacity = static_cast<T*>(listStart[CapacityPtrOffset]);
            const wIndex oldCount = end - begin;

            const bool aliasesSelf = (&value >= begin && &value < end);

            if (end == capacity)
            {
                ReallocateKnownList<T>(listStart, CalculateNextCapacity(oldCount + 1, capacity - begin));
                end = static_cast<T*>(endVoid);
            }

            if constexpr (std::is_trivially_copy_constructible_v<T>)
            {
                if (aliasesSelf)
                {
                    T tmp = value;
                    std::construct_at(end, tmp);
                }
                else
                {
                    std::construct_at(end, value);
                }
            }
            else
            {
                if (aliasesSelf)
                {
                    T tmp = value;
                    std::construct_at(end, std::move_if_noexcept(tmp));
                }
                else
                {
                    std::construct_at(end, value);
                }
            }

            endVoid = end + 1;
            return oldCount;
        }

        template<typename T>
        wIndex KnownListAdd(void** listStart, T&& value)
        {
            void*& endVoid = listStart[EndPtrOffset];
            T* end = static_cast<T*>(endVoid);
            const T* const begin = static_cast<T*>(listStart[BeginPtrOffset]);
            const T* const capacity = static_cast<T*>(listStart[CapacityPtrOffset]);
            const wIndex oldCount = end - begin;

            if (end == capacity)
            {
                ReallocateKnownList<T>(listStart, CalculateNextCapacity(oldCount + 1, capacity - begin));
                end = static_cast<T*>(endVoid);
            }

            std::construct_at(end, std::move_if_noexcept(value));

            endVoid = end + 1;
            return oldCount;
        }

        template<typename T, typename... Args>
        wIndex KnownListEmplace(void** listStart, Args&&... args)
        {
            void*& endVoid = listStart[EndPtrOffset];
            T* end = static_cast<T*>(endVoid);
            const T* const begin = static_cast<T*>(listStart[BeginPtrOffset]);
            const T* const capacity = static_cast<T*>(listStart[CapacityPtrOffset]);
            const wIndex oldCount = end - begin;

            if (end == capacity)
            {
                ReallocateKnownList<T>(listStart, CalculateNextCapacity(oldCount + 1, capacity - begin));
                end = static_cast<T*>(endVoid);
            }

            std::construct_at(end, std::forward<Args>(args)...);

            endVoid = end + 1;
            return oldCount;
        }

        template<typename T>
        void ReallocateKnownList(void** listStart, wIndex newCapacity)
        {
            void*& endVoid = listStart[EndPtrOffset];
            void*& beginVoid = listStart[BeginPtrOffset];
            const T* const end = static_cast<T*>(endVoid);
            const T* const begin = static_cast<T*>(beginVoid);
            const std::size_t oldCount = end - begin;

            T* newKnownList = static_cast<T*>(
                ::operator new(newCapacity * sizeof(T), std::align_val_t(alignof(T)))
            );

            if (oldCount)
            {
                std::memcpy(newKnownList, beginVoid, oldCount * sizeof(T));
            }

            if (beginVoid)
            {
                ::operator delete(beginVoid, std::align_val_t(alignof(T)));
            }

            beginVoid = newKnownList;
            endVoid = newKnownList + oldCount;
            listStart[CapacityPtrOffset] = newKnownList + newCapacity;
        }

        template<typename T>
        void DestroyKnownList(void** listStart)
        {
            void*& beginVoid = listStart[BeginPtrOffset];
            if (beginVoid)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    T* const begin = static_cast<T*>(beginVoid);
                    T* const end = static_cast<T*>(listStart[EndPtrOffset]);
                    for (T* element = begin; element != end; ++element)
                    {
                        std::destroy_at(element);
                    }
                }
                ::operator delete(beginVoid, std::align_val_t(alignof(T)));
            }
        }

        template<typename T>
        wIndex KnownListCount(void** listStart) const
        {
            T* const end = static_cast<T*>(listStart[EndPtrOffset]);
            T* const begin = static_cast<T*>(listStart[BeginPtrOffset]);
            return end - begin;
        }

        template<typename T>
        static T* EmptySentinel() noexcept
        {
            alignas(T) static std::byte raw[sizeof(T)];
            return reinterpret_cast<T*>(&raw);
        }

        void ReallocateScenes(wIndex sceneCapacity);
        void DeleteSceneContent(std::size_t sceneStartPtrIndex);

        void ReallocateComponentList(std::size_t componentListStartPtrIndex, ComponentTypeIndex componentTypeIndex, wIndex newCapacity);
        void DestroyComponentList(std::size_t sceneStartPtrIndex, ComponentTypeIndex componentTypeIndex);

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
        inline std::size_t GetSceneStartPtrIndex(SceneIndex sceneIndex) const noexcept { return (sceneIndex - 1) * GetSceneSizePointers(); }

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