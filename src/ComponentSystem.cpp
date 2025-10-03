#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSystem.hpp"

// A scene is invalid if the first slotCount is greater than the first capacity
// slotCount: 1
// capacity: 0
// invalid if slotCount > capacity
// exists if slotCount <= capacity

namespace wCore
{
    //static constexpr std::uintptr_t InvalidComponentListTrue = static_cast<std::uintptr_t>(1);

    ComponentSystem::ComponentSystem(Application& app) noexcept
        : m_app(app), m_componentSetup(), m_currentComponentTypeCount(0),
        m_scenes(nullptr), m_sceneSlotCount(0), m_sceneSlotCapacity(0),
        m_sceneFreeList()
    {
    }

    ComponentSystem::~ComponentSystem() noexcept
    {
        /*if (m_componentLists)
        {
            for (SceneIndex sceneIndex = SceneIndexStart; sceneIndex <= m_sceneData.size(); ++sceneIndex)
            {
                const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
                if (m_componentLists[sceneStartListIndex].begin)
                {
                    DeleteSceneContent(sceneStartListIndex);
                }
            }
            ::operator delete(m_componentLists, std::align_val_t(alignof(ComponentSetup::PageListHeader)));
        }*/
    }

    void ComponentSystem::ReserveScenes(wIndex minCapacity)
    {
        if (minCapacity > m_sceneSlotCapacity)
        {
            ReallocateScenes(minCapacity);
        }
    }

    wIndex ComponentSystem::CreateScene(std::string_view name)
    {
        const std::size_t pointerCountPerScene = GetPointerCountPerScene(m_currentComponentTypeCount);
        const std::size_t indexCountPerScene = GetIndexCountPerScene(m_currentComponentTypeCount);

        if (m_sceneFreeList.Empty())
        {
            if (m_sceneSlotCount == m_sceneSlotCapacity)
            {
                ReallocateScenes(CalculateNextCapacity(m_sceneSlotCapacity));
            }

            const std::size_t sceneStartPointerIndex = m_sceneSlotCount * pointerCountPerScene;
            const std::size_t sceneStartIndexIndex = m_sceneSlotCount * indexCountPerScene;
            const std::size_t sceneStartFreeListIndex = m_sceneSlotCount * m_currentComponentTypeCount;

            std::memset(m_ptrs + sceneStartPointerIndex, 0, sizeof(void*) * pointerCountPerScene);
            std::memset(m_indexes + sceneStartIndexIndex, 0, sizeof(wIndex) * indexCountPerScene);
            std::memset(m_freeLists + sceneStartFreeListIndex, 0, sizeof(wUtils::RelocatableFreeListHeader<wIndex>) * m_currentComponentTypeCount);
            std::construct_at(m_sceneData + m_sceneSlotCount);

            return m_sceneSlotCount++;
        }
        const wIndex sceneIndex = m_sceneFreeList.Remove();

        // Reset the memory
        const std::size_t sceneStartPointerIndex = (sceneIndex - 1) * pointerCountPerScene;
        const std::size_t sceneStartIndexIndex = (sceneIndex - 1) * indexCountPerScene;
        const std::size_t sceneStartFreeListIndex = (sceneIndex - 1) * m_currentComponentTypeCount;

        W_ASSERT(m_indexes[sceneStartIndexIndex] > m_indexes[sceneStartIndexIndex + m_currentComponentTypeCount], "A Free/Invalid ComponentList Should the first slotCount be less than the first capacity. Expected: slotCount: 1, capacity: 0, slotCount: {}, capacity {}", m_indexes[sceneStartIndexIndex], m_indexes[sceneStartIndexIndex + m_currentComponentTypeCount]);

        std::memset(m_ptrs + sceneStartPointerIndex, 0, sizeof(void*) * pointerCountPerScene);
        std::memset(m_indexes + sceneStartIndexIndex, 0, sizeof(wIndex) * indexCountPerScene);
        std::memset(m_freeLists + sceneStartFreeListIndex, 0, sizeof(wUtils::RelocatableFreeListHeader<wIndex>) * m_currentComponentTypeCount);
        std::construct_at(m_sceneData + sceneIndex - 1);

        return sceneIndex;
    }

    /*void ComponentSystem::DestroyScene(wIndex sceneIndex)
    {
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        ComponentSetup::ListHeader& firstElement = m_componentLists[sceneStartListIndex];
        if (firstElement.begin)
        {
            DeleteSceneContent(sceneStartListIndex);
            firstElement.begin = nullptr;
            m_sceneFreeList.push_back(sceneIndex);
        }
    }*/

    [[nodiscard]] inline bool ComponentSystem::SceneExists(SceneIndex sceneIndex) const noexcept
    {
        const std::size_t sceneStartIndexIndex = (sceneIndex - 1) * GetIndexCountPerScene(m_currentComponentTypeCount);
        return m_indexes[sceneStartIndexIndex] <= m_indexes[sceneStartIndexIndex + m_currentComponentTypeCount];
    }

    void ComponentSystem::ReserveComponents(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex minCapacity)
    {
        const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];
        const std::size_t sceneStartIndexIndex = (sceneIndex - 1) * GetIndexCountPerScene(m_currentComponentTypeCount);
        const wIndex slotCount = m_indexes[sceneStartIndexIndex + componentTypeIndex - 1];
        const wIndex capacity = m_indexes[sceneStartIndexIndex + m_currentComponentTypeCount + componentTypeIndex - 1];
        const wUtils::RelocatableFreeListHeader<wIndex> freeList = m_freeLists[sceneStartIndexIndex + m_currentComponentTypeCount + componentTypeIndex - 1];

        W_IF_UNLIKELY(type.pageSize)
        {
            if (minCapacity > capacity * type.pageSize - freeList.Count()) {
                const std::size_t sceneStartPointerIndex = (sceneIndex - 1) * GetPointerCountPerScene(m_currentComponentTypeCount);
                type.reallocatePages(m_ptrs[sceneStartPointerIndex], componentCapacity);
            }
        }
        W_ELSE_LIKELY
        {
            if (minCapacity > slotCapacity - freeList.Count()) {
                type.reallocateComponents(, componentCapacity);
            }
        }
    }

    ComponentIndex ComponentSystem::CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex)
    {
        //const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        //const wIndex listIndex = m_componentSetup.m_types[componentTypeIndex - 1].create(m_componentLists[sceneStartListIndex + GetComponentListOffset(componentTypeIndex)], m_app);
        return 0;// ComponentSetup::EmplaceKnownList<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset], 0, listIndex) + 1;
    }

    /*wIndex ComponentSystem::GetComponentCount(ComponentTypeIndex componentTypeIndex, wIndex sceneIndex) const
    {
        const ComponentSetup::ListHeader& header = GetComponentListHeader(sceneIndex, componentTypeIndex);
        return (header.End<std::byte>() - header.Begin<std::byte>()) / m_componentSetup.m_types[componentTypeIndex - 1].size;
    }

    wIndex ComponentSystem::GetComponentCapacity(ComponentTypeIndex componentTypeIndex, wIndex sceneIndex) const
    {
        const ComponentSetup::ListHeader& header = GetComponentListHeader(sceneIndex, componentTypeIndex);
        return (header.Capacity<std::byte>() - header.Begin<std::byte>()) / m_componentSetup.m_types[componentTypeIndex - 1].size;
    }*/

    void ComponentSystem::ReallocateScenes(wIndex newCapacity)
    {
        m_sceneSlotCapacity = newCapacity;

        if (m_scenes)
        {
            m_currentComponentTypeCount = m_componentSetup.GetComponentTypeCount();
        }

        const wIndex ptrCountPerScene = GetPointerCountPerScene(m_currentComponentTypeCount);
        const wIndex indexCountPerScene = GetIndexCountPerScene(m_currentComponentTypeCount);

        std::size_t offset = 0;

        offset = wUtils::AlignUp(offset, alignof(void*));
        const std::size_t ptrOffset = offset;
        offset += ptrCountPerScene * newCapacity * sizeof(void*);

        offset = wUtils::AlignUp(offset, alignof(wIndex));
        const std::size_t indexOffset = offset;
        offset += indexCountPerScene * newCapacity * sizeof(wIndex);

        offset = wUtils::AlignUp(offset, alignof(wUtils::RelocatableFreeListHeader<wIndex>));
        const std::size_t freeListOffset = offset;
        offset += m_currentComponentTypeCount * newCapacity * sizeof(wUtils::RelocatableFreeListHeader<wIndex>);

        offset = wUtils::AlignUp(offset, alignof(SceneData));
        const std::size_t sceneDataOffset = offset;
        offset += newCapacity * sizeof(SceneData);

        constexpr std::size_t alignment = std::max(std::max(alignof(void*), alignof(wIndex)), std::max(alignof(wUtils::RelocatableFreeListHeader<wIndex>), alignof(SceneData)));
        if (m_scenes)
        {
            std::byte* newScenes = static_cast<std::byte*>(
                ::operator new(offset, std::align_val_t(alignof(alignment)))
            );

            void** newPtrs = reinterpret_cast<void**>(newScenes + ptrOffset);
            wIndex* newIndexes = reinterpret_cast<wIndex*>(newScenes + indexOffset);
            wUtils::RelocatableFreeListHeader<wIndex>* newFreeLists = reinterpret_cast<wUtils::RelocatableFreeListHeader<wIndex>*>(newScenes + freeListOffset);
            SceneData* newSceneData = reinterpret_cast<SceneData*>(newScenes + sceneDataOffset);

            if (m_sceneSlotCount)
            {
                std::memcpy(newPtrs, m_ptrs, m_sceneSlotCount * ptrCountPerScene * sizeof(void*));
                std::memcpy(newIndexes, m_indexes, m_sceneSlotCount * indexCountPerScene * sizeof(wIndex));
                std::memcpy(newFreeLists, m_freeLists, m_sceneSlotCount * sizeof(wUtils::RelocatableFreeListHeader<wIndex>));
                std::memcpy(newSceneData, m_sceneData, m_sceneSlotCount * sizeof(SceneData));
            }

            ::operator delete(m_scenes, std::align_val_t(alignof(alignment)));

            m_scenes = newScenes;
        }
        else
        {
            m_scenes = static_cast<std::byte*>(
                ::operator new(offset, std::align_val_t(alignment))
            );
        }

        m_ptrs = reinterpret_cast<void**>(m_scenes + ptrOffset);
        m_indexes = reinterpret_cast<wIndex*>(m_scenes + indexOffset);
        m_freeLists = reinterpret_cast<wUtils::RelocatableFreeListHeader<wIndex>*>(m_scenes + freeListOffset);
        m_sceneData = reinterpret_cast<SceneData*>(m_scenes + sceneDataOffset);
    }

    /*void ComponentSystem::DeleteSceneContent(std::size_t sceneStartListIndex) noexcept
    {
        DestroyKnownList<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset]);
        DestroyKnownList<std::string>(m_componentLists[sceneStartListIndex + ComponentNameListOffset]);
        for (wIndex componentTypeIndex = ComponentTypeIndexStart; componentTypeIndex <= m_currentComponentTypeCount; ++componentTypeIndex)
        {
            DestroyComponentList(sceneStartListIndex, componentTypeIndex);
        }
    }

    void ComponentSystem::DestroyComponentList(std::size_t sceneStartListIndex, ComponentTypeIndex componentTypeIndex)
    {
        ComponentSetup::ListHeader& header = m_componentLists[sceneStartListIndex + GetComponentListOffset(componentTypeIndex)];
        if (header.begin != m_componentSetup.m_emptySentinals[componentTypeIndex - 1])
        {
            m_componentSetup.m_types[componentTypeIndex - 1].destroy(header);
        }
    }*/
}