#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSystem.hpp"

namespace wCore
{
    //static constexpr std::uintptr_t InvalidComponentListTrue = static_cast<std::uintptr_t>(1);

    ComponentSystem::ComponentSystem(Application& app) noexcept
        : m_app(app), m_componentSetup(), m_currentComponentListCount(0),
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

    SceneHandle ComponentSystem::CreateScene(std::string_view name)
    {
        if (m_sceneFreeList.Empty())
        {
            if (m_sceneSlotCount == m_sceneSlotCapacity)
            {
                ReallocateScenes(CalculateNextCapacity(m_sceneSlotCapacity));
            }

            const std::size_t sceneStartComponentIndex = m_sceneSlotCount * GetCurrentComponentListCount();
            const std::size_t sceneStartPageIndex = m_sceneSlotCount * GetCurrentPageListCount();

            std::memset(m_componentListsHot + sceneStartComponentIndex, 0, sizeof(ComponentListHeaderHot) * GetCurrentComponentListCount());
            std::memset(m_pageListsHot + sceneStartPageIndex, 0, sizeof(PageListHeaderHot) * GetCurrentPageListCount());
            std::memset(m_componentListsCold + sceneStartComponentIndex, 0, sizeof(ComponentListHeaderCold) * GetCurrentComponentListCount());
            std::memset(m_pageListsCold + sceneStartPageIndex, 0, sizeof(PageListHeaderCold) * GetCurrentPageListCount());
            std::construct_at(m_sceneData + m_sceneSlotCount);

            return SceneHandle(m_sceneSlotCount++, *std::construct_at(m_sceneGenerations + m_sceneSlotCount));
        }
        const SceneIndex sceneIndex = m_sceneFreeList.Remove();

        // Reset the memory
        const std::size_t sceneStartComponentIndex = (sceneIndex - 1) * GetCurrentComponentListCount();
        const std::size_t sceneStartPageIndex = (sceneIndex - 1) * GetCurrentComponentListCount();

        std::memset(m_componentListsHot + sceneStartComponentIndex, 0, sizeof(ComponentListHeaderHot) * GetCurrentComponentListCount());
        std::memset(m_pageListsHot + sceneStartPageIndex, 0, sizeof(PageListHeaderHot) * GetCurrentPageListCount());
        std::memset(m_componentListsCold + sceneStartComponentIndex, 0, sizeof(ComponentListHeaderCold) * GetCurrentComponentListCount());
        std::memset(m_pageListsCold + sceneStartPageIndex, 0, sizeof(PageListHeaderCold) * GetCurrentPageListCount());
        std::construct_at(m_sceneData + sceneIndex - 1);

        return SceneHandle(sceneIndex, m_sceneGenerations[sceneIndex - 1]);
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

    void ComponentSystem::ReserveComponents(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex minCapacity)
    {
        const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];
        //const std::size_t sceneStartIndexIndex = (sceneIndex - 1) * GetIndexCountPerScene(m_currentComponentListCount);
        //const wIndex slotCount = m_indexes[sceneStartIndexIndex + componentTypeIndex - 1];
        //wIndex& capacity = m_indexes[sceneStartIndexIndex + m_currentComponentListCount + componentTypeIndex - 1];

        W_IF_UNLIKELY(type.pageSize)
        {
            const std::size_t sceneStartPageIndex = m_sceneSlotCount * GetCurrentPageListCount();
            const PageListHeaderCold& pageListHeaderCold = m_pageListsCold[sceneStartPageIndex - componentTypeIndex - 1];
            if (minCapacity > pageListHeaderCold.pageCount * type.pageSize) {
                const wIndex minPageCount = wUtils::IntDivCeil(minCapacity, type.pageSize);
                //const std::size_t sceneStartPointerIndex = (sceneIndex - 1) * GetPointerCountPerScene(m_currentComponentListCount);
                type.reallocatePages(m_ptrs[sceneStartPointerIndex], capacity, minPageCount);
            }
        }
        W_ELSE_LIKELY
        {
            if (minCapacity > capacity) {
                const std::size_t sceneStartPointerIndex = (sceneIndex - 1) * GetPointerCountPerScene(m_currentComponentListCount);
                type.reallocateComponents(m_ptrs[sceneStartPointerIndex], slotCount, capacity, minCapacity);
            }
        }
    }

    ComponentIndex ComponentSystem::CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex)
    {
        return m_componentSetup.m_types[componentTypeIndex - 1].create(m_componentLists[sceneStartListIndex + GetComponentListOffset(componentTypeIndex)], m_app);
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
            m_currentComponentListCount = m_componentSetup.m_componentListCount;
        }

        std::size_t offset = 0;

        offset = wUtils::AlignUp(offset, alignof(ComponentListHeaderHot));
        const std::size_t componentListHotOffset = offset;
        offset += GetCurrentComponentListCount() * newCapacity * sizeof(ComponentListHeaderHot);

        offset = wUtils::AlignUp(offset, alignof(PageListHeaderHot));
        const std::size_t pageListHotOffset = offset;
        offset += GetCurrentPageListCount() * newCapacity * sizeof(PageListHeaderHot);

        offset = wUtils::AlignUp(offset, alignof(SceneGeneration));
        const std::size_t sceneGenerationOffset = offset;
        offset += newCapacity * sizeof(SceneGeneration);

        offset = wUtils::AlignUp(offset, alignof(ComponentListHeaderCold));
        const std::size_t componentListColdOffset = offset;
        offset += GetCurrentComponentListCount() * newCapacity * sizeof(ComponentListHeaderCold);

        offset = wUtils::AlignUp(offset, alignof(PageListHeaderCold));
        const std::size_t pageListColdOffset = offset;
        offset += GetCurrentPageListCount() * newCapacity * sizeof(PageListHeaderCold);

        offset = wUtils::AlignUp(offset, alignof(SceneData));
        const std::size_t sceneDataOffset = offset;
        offset += newCapacity * sizeof(SceneData);

        constexpr std::size_t alignment = wUtils::MaxAlignOf<ComponentListHeaderHot, PageListHeaderHot, SceneGeneration, ComponentListHeaderCold, PageListHeaderCold, SceneData>;
        if (m_scenes)
        {
            std::byte* newScenes = static_cast<std::byte*>(
                ::operator new(offset, std::align_val_t(alignof(alignment)))
            );

            ComponentListHeaderHot* newComponentListsHot = reinterpret_cast<ComponentListHeaderHot*>(newScenes + componentListHotOffset);
            PageListHeaderHot* newPageListsHot = reinterpret_cast<PageListHeaderHot*>(newScenes + pageListHotOffset);
            SceneGeneration* newSceneGenerations = reinterpret_cast<SceneGeneration*>(newScenes + sceneGenerationOffset);
            ComponentListHeaderCold* newComponentListsCold = reinterpret_cast<ComponentListHeaderCold*>(newScenes + componentListColdOffset);
            PageListHeaderCold* newPageListsCold = reinterpret_cast<PageListHeaderCold*>(newScenes + pageListColdOffset);
            SceneData* newSceneData = reinterpret_cast<SceneData*>(newScenes + sceneDataOffset);

            if (m_sceneSlotCount)
            {
                std::memcpy(newComponentListsHot, m_componentListsHot, m_sceneSlotCount * GetCurrentComponentListCount() * sizeof(ComponentListHeaderHot));
                std::memcpy(newPageListsHot, m_pageListsHot, m_sceneSlotCount * GetCurrentPageListCount() * sizeof(PageListHeaderHot));
                std::memcpy(newSceneGenerations, m_sceneGenerations, m_sceneSlotCount * sizeof(SceneGeneration));
                std::memcpy(newComponentListsCold, m_componentListsCold, m_sceneSlotCount * GetCurrentComponentListCount() * sizeof(ComponentListHeaderCold));
                std::memcpy(newPageListsCold, m_pageListsCold, m_sceneSlotCount * GetCurrentPageListCount() * sizeof(ComponentListHeaderCold));
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

        m_componentListsHot = reinterpret_cast<ComponentListHeaderHot*>(m_scenes + componentListHotOffset);
        m_pageListsHot = reinterpret_cast<PageListHeaderHot*>(m_scenes + pageListHotOffset);
        m_sceneGenerations = reinterpret_cast<SceneGeneration*>(m_scenes + sceneGenerationOffset);
        m_componentListsCold = reinterpret_cast<ComponentListHeaderCold*>(m_scenes + componentListColdOffset);
        m_pageListsCold = reinterpret_cast<PageListHeaderCold*>(m_scenes + pageListColdOffset);
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