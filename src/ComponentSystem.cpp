#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSystem.hpp"

namespace wCore
{
    //static constexpr std::uintptr_t InvalidComponentListTrue = static_cast<std::uintptr_t>(1);

    ComponentSystem::ComponentSystem(Application& app) noexcept
        : m_app(app), m_componentSetup(),
        m_scenes(nullptr), m_componentListsHot(nullptr), m_pageListsHot(nullptr), m_sceneGenerations(nullptr), m_componentListsCold(nullptr), m_pageListsCold(nullptr),
        m_sceneSlotCount(0), m_sceneSlotCapacity(0),
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

            const std::size_t sceneStartComponentIndex = m_sceneSlotCount * m_componentSetup.GetCurrentComponentListCount();
            const std::size_t sceneStartPageIndex = m_sceneSlotCount * m_componentSetup.GetCurrentPageListCount();

            std::memset(m_componentListsHot + sceneStartComponentIndex, 0, sizeof(ComponentSetup::ComponentListHeaderHot) * GetCurrentComponentListCount());
            std::memset(m_pageListsHot + sceneStartPageIndex, 0, sizeof(ComponentSetup::PageListHeaderHot) * GetCurrentPageListCount());
            std::memset(m_componentListsCold + sceneStartComponentIndex, 0, sizeof(ComponentSetup::ComponentListHeaderCold) * GetCurrentComponentListCount());
            std::memset(m_pageListsCold + sceneStartPageIndex, 0, sizeof(ComponentSetup::PageListHeaderCold) * GetCurrentPageListCount());
            std::construct_at(m_sceneData + m_sceneSlotCount);

            return SceneHandle(m_sceneSlotCount++, *std::construct_at(m_sceneGenerations + m_sceneSlotCount));
        }
        const SceneIndex sceneIndex = m_sceneFreeList.Remove();

        // Reset the memory
        const std::size_t sceneStartComponentIndex = (sceneIndex - 1) * m_componentSetup.GetCurrentComponentListCount();
        const std::size_t sceneStartPageIndex = (sceneIndex - 1) * m_componentSetup.GetCurrentComponentListCount();

        std::memset(m_componentListsHot + sceneStartComponentIndex, 0, sizeof(ComponentSetup::ComponentListHeaderHot) * GetCurrentComponentListCount());
        std::memset(m_pageListsHot + sceneStartPageIndex, 0, sizeof(ComponentSetup::PageListHeaderHot) * GetCurrentPageListCount());
        std::memset(m_componentListsCold + sceneStartComponentIndex, 0, sizeof(ComponentSetup::ComponentListHeaderCold) * GetCurrentComponentListCount());
        std::memset(m_pageListsCold + sceneStartPageIndex, 0, sizeof(ComponentSetup::PageListHeaderCold) * GetCurrentPageListCount());
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
        if (type.pageSize)
        {
            const std::size_t sceneStartPageIndex = (sceneIndex - 1) * m_createCtx.GetCurrentPageListCount();
            const std::size_t pageListHeaderIndex = sceneStartPageIndex + type.listIndex;
            ComponentSetup::PageListHeaderCold& pageListHeaderCold = m_createCtx.pageListsCold[pageListHeaderIndex];
            if (minCapacity > pageListHeaderCold.pageCount * type.pageSize)
            {
                const wIndex minPageCount = wUtils::IntDivCeil(minCapacity, type.pageSize);
                type.reallocatePages(m_createCtx.pageListsHot[pageListHeaderIndex], pageListHeaderCold, minPageCount);
            }
        }
        else
        {
            const std::size_t sceneStartComponentIndex = (sceneIndex - 1) * m_createCtx.GetCurrentComponentListCount();
            const std::size_t componentListHeaderIndex = sceneStartComponentIndex + type.listIndex - 1;
            ComponentSetup::ComponentListHeaderCold& componentListHeaderCold = m_createCtx.componentListsCold[componentListHeaderIndex];
            if (minCapacity > componentListHeaderCold.capacity)
            {
                type.reallocateComponents(m_createCtx.componentListsHot[componentListHeaderIndex], componentListHeaderCold, minCapacity);
            }
        }
    }

    ComponentHandleAny ComponentSystem::CreateComponent(ComponentTypeIndex componentTypeIndex, SceneHandle scene)
    {
        const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];
        auto [componentIndex, generation] = type.create(scene.sceneIndex, m_createCtx, m_app);
        return ComponentHandleAny(componentTypeIndex, scene, componentIndex, generation);
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

        if (!m_scenes) // TODO: Check if this if should be !
        {
            m_createCtx.UpdateCurrentComponentListCount(m_componentSetup.GetComponentTypeCount(), m_componentSetup.m_componentListCount);
        }

        std::size_t offset = 0;

        offset = wUtils::AlignUp(offset, alignof(ComponentSetup::ComponentListHeaderHot));
        const std::size_t componentListHotOffset = offset;
        offset += m_createCtx.GetCurrentComponentListCount() * newCapacity * sizeof(ComponentSetup::ComponentListHeaderHot);

        offset = wUtils::AlignUp(offset, alignof(ComponentSetup::PageListHeaderHot));
        const std::size_t pageListHotOffset = offset;
        offset += m_createCtx.GetCurrentPageListCount() * newCapacity * sizeof(ComponentSetup::PageListHeaderHot);

        offset = wUtils::AlignUp(offset, alignof(SceneGeneration));
        const std::size_t sceneGenerationOffset = offset;
        offset += newCapacity * sizeof(SceneGeneration);

        offset = wUtils::AlignUp(offset, alignof(ComponentSetup::ComponentListHeaderCold));
        const std::size_t componentListColdOffset = offset;
        offset += m_createCtx.GetCurrentComponentListCount() * newCapacity * sizeof(ComponentSetup::ComponentListHeaderCold);

        offset = wUtils::AlignUp(offset, alignof(ComponentSetup::PageListHeaderCold));
        const std::size_t pageListColdOffset = offset;
        offset += m_createCtx.GetCurrentPageListCount() * newCapacity * sizeof(ComponentSetup::PageListHeaderCold);

        offset = wUtils::AlignUp(offset, alignof(SceneData));
        const std::size_t sceneDataOffset = offset;
        offset += newCapacity * sizeof(SceneData);

        constexpr std::size_t alignment = wUtils::MaxAlignOf<ComponentSetup::ComponentListHeaderHot, ComponentSetup::PageListHeaderHot, SceneGeneration, ComponentSetup::ComponentListHeaderCold, ComponentSetup::PageListHeaderCold, SceneData>;
        if (m_scenes)
        {
            std::byte* newScenes = static_cast<std::byte*>(
                ::operator new(offset, std::align_val_t(alignment))
            );

            ComponentSetup::ComponentListHeaderHot* newComponentListsHot = reinterpret_cast<ComponentSetup::ComponentListHeaderHot*>(newScenes + componentListHotOffset);
            ComponentSetup::PageListHeaderHot* newPageListsHot = reinterpret_cast<ComponentSetup::PageListHeaderHot*>(newScenes + pageListHotOffset);
            SceneGeneration* newSceneGenerations = reinterpret_cast<SceneGeneration*>(newScenes + sceneGenerationOffset);
            ComponentSetup::ComponentListHeaderCold* newComponentListsCold = reinterpret_cast<ComponentSetup::ComponentListHeaderCold*>(newScenes + componentListColdOffset);
            ComponentSetup::PageListHeaderCold* newPageListsCold = reinterpret_cast<ComponentSetup::PageListHeaderCold*>(newScenes + pageListColdOffset);
            SceneData* newSceneData = reinterpret_cast<SceneData*>(newScenes + sceneDataOffset);

            if (m_sceneSlotCount)
            {
                std::memcpy(newComponentListsHot, m_createCtx.componentListsHot, m_sceneSlotCount * m_createCtx.GetCurrentComponentListCount() * sizeof(ComponentSetup::ComponentListHeaderHot));
                std::memcpy(newPageListsHot, m_createCtx.pageListsHot, m_sceneSlotCount * m_createCtx.GetCurrentPageListCount() * sizeof(ComponentSetup::PageListHeaderHot));
                std::memcpy(newSceneGenerations, m_sceneGenerations, m_sceneSlotCount * sizeof(SceneGeneration));
                std::memcpy(newComponentListsCold, m_createCtx.componentListsCold, m_sceneSlotCount * m_createCtx.GetCurrentComponentListCount() * sizeof(ComponentSetup::ComponentListHeaderCold));
                std::memcpy(newPageListsCold, m_createCtx.pageListsCold, m_sceneSlotCount * m_createCtx.GetCurrentPageListCount() * sizeof(ComponentSetup::ComponentListHeaderCold));
                std::memcpy(newSceneData, m_sceneData, m_sceneSlotCount * sizeof(SceneData));
            }

            ::operator delete(m_scenes, std::align_val_t(alignment));

            m_scenes = newScenes;
        }
        else
        {
            m_scenes = static_cast<std::byte*>(
                ::operator new(offset, std::align_val_t(alignment))
            );
        }

        m_createCtx.componentListsHot = reinterpret_cast<ComponentSetup::ComponentListHeaderHot*>(m_scenes + componentListHotOffset);
        m_createCtx.pageListsHot = reinterpret_cast<ComponentSetup::PageListHeaderHot*>(m_scenes + pageListHotOffset);
        m_sceneGenerations = reinterpret_cast<SceneGeneration*>(m_scenes + sceneGenerationOffset);
        m_createCtx.componentListsCold = reinterpret_cast<ComponentSetup::ComponentListHeaderCold*>(m_scenes + componentListColdOffset);
        m_createCtx.pageListsCold = reinterpret_cast<ComponentSetup::PageListHeaderCold*>(m_scenes + pageListColdOffset);
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