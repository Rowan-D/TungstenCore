#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSystem.hpp"

// A scene is invalid if the first void* is nullptr, empty if sentinel, contains something if neather.

namespace wCore
{
    static constexpr std::uintptr_t InvalidComponentListTrue = static_cast<std::uintptr_t>(1);

    ComponentSystem::ComponentSystem(Application& app)
        : m_app(app), m_componentSetup(), m_currentComponentTypeCount(0),
        m_emptyComponentLists(nullptr), m_componentLists(nullptr), m_componentListsCapacity(0),
        m_sceneData(), m_sceneFreeList()
    {
    }

    ComponentSystem::~ComponentSystem()
    {
        if (m_componentLists)
        {
            for (SceneIndex sceneIndex = SceneIndexStart; sceneIndex <= m_sceneData.size(); ++sceneIndex)
            {
                const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
                if (m_componentLists[sceneStartListIndex].begin)
                {
                    DeleteSceneContent(sceneStartListIndex);
                }
            }
            W_ASSERT(m_emptyComponentLists, "m_emptyComponentList should not be nullptr if m_componentLists has been created!");
            ::operator delete(m_emptyComponentLists, std::align_val_t(alignof(ComponentSetup::ListHeader)));
            ::operator delete(m_componentLists, std::align_val_t(alignof(ComponentSetup::ListHeader)));
        }
    }

    void ComponentSystem::ReserveScenes(wIndex sceneCapacity)
    {
        m_sceneData.reserve(sceneCapacity);
        if (sceneCapacity > m_componentListsCapacity)
        {
            ReallocateScenes(sceneCapacity);
        }
    }

    wIndex ComponentSystem::CreateScene()
    {
        const std::size_t sceneSizeLists = GetSceneSizeLists();
        if (m_sceneFreeList.empty())
        {
            if (m_sceneData.size() == m_componentListsCapacity)
            {
                ReallocateScenes(ComponentSetup::CalculateNextCapacity(m_sceneData.size() + 1, m_componentListsCapacity));
            }

            const std::size_t sceneStartListIndex = m_sceneData.size() * sceneSizeLists;
            std::memcpy(m_componentLists + sceneStartListIndex, m_emptyComponentLists, sceneSizeLists * sizeof(void*));

            m_sceneData.emplace_back();

            return m_sceneData.size();
        }
        const wIndex sceneIndex = m_sceneFreeList.back();
        m_sceneFreeList.pop_back();

        // Reset the memory to empty sentinels
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        W_ASSERT(m_componentLists[sceneStartListIndex].begin, "A Free/Invalid ComponentList Should Have the first pointer be false (Set to nullptr).");
        std::memcpy(m_componentLists + sceneStartListIndex, m_emptyComponentLists, sceneSizeLists * sizeof(ComponentSetup::ListHeader));

        m_sceneData[sceneIndex - 1] = SceneData{};

        return sceneIndex;
    }

    void ComponentSystem::DestroyScene(wIndex sceneIndex)
    {
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        ComponentSetup::ListHeader& firstElement = m_componentLists[sceneStartListIndex];
        if (firstElement.begin)
        {
            DeleteSceneContent(sceneStartListIndex);
            firstElement.begin = nullptr;
            m_sceneFreeList.push_back(sceneIndex);
        }
    }

    wIndex ComponentSystem::GetComponentCount(uint32_t sceneIndex) const
    {
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        return GetKnownListCount<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset]);
    }

    void ComponentSystem::ReserveComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex componentCapacity)
    {
        const std::size_t componentListStartListIndex = GetComponentStartListIndex(sceneIndex, componentTypeIndex);
        ComponentSetup::ListHeader& header = m_componentLists[componentListStartListIndex];

        const std::byte* componentListCapacity = header.Capacity<std::byte>();
        const std::byte* componentListBegin = header.Begin<std::byte>();

        const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];
        if (componentCapacity * type.size > componentListCapacity - componentListBegin)
        {
            type.reallocate(header, componentCapacity);
        }
    }

    ComponentIndex ComponentSystem::CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex)
    {
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        const wIndex listIndex = m_componentSetup.m_types[componentTypeIndex - 1].create(m_componentLists[sceneStartListIndex + GetComponentListOffset(componentTypeIndex)], m_app);
        return ComponentSetup::EmplaceKnownList<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset], 0, listIndex);
    }

    void ComponentSystem::ReallocateScenes(wIndex newCapacity)
    {
        m_componentListsCapacity = newCapacity;

        if (m_componentLists)
        {
            const std::size_t sceneSizeBytes = GetSceneSizeBytes();
            ComponentSetup::ListHeader* newComponentLists = static_cast<ComponentSetup::ListHeader*>(
                ::operator new(newCapacity * sceneSizeBytes, std::align_val_t(alignof(ComponentSetup::ListHeader)))
            );

            if (!m_sceneData.empty())
            {
                std::memcpy(newComponentLists, m_componentLists, m_sceneData.size() * sceneSizeBytes);
            }

            ::operator delete(m_componentLists, std::align_val_t(alignof(ComponentSetup::ListHeader*)));

            m_componentLists = newComponentLists;
        }
        else
        {
            m_currentComponentTypeCount = m_componentSetup.GetComponentTypeCount();
            const std::size_t sceneSizeBytes = GetSceneSizeBytes();

            m_emptyComponentLists = static_cast<ComponentSetup::ListHeader*>(
                ::operator new(sceneSizeBytes, std::align_val_t(alignof(ComponentSetup::ListHeader)))
            );

            void* const componentSentinel = ComponentSetup::EmptySentinel<Component>();
            void* const standardStringSentinel = ComponentSetup::EmptySentinel<std::string>();

            m_emptyComponentLists[0] = { componentSentinel, componentSentinel, componentSentinel };
            m_emptyComponentLists[1] = { standardStringSentinel, standardStringSentinel, standardStringSentinel };

            ComponentSetup::ListHeader* dst = m_emptyComponentLists + KnownListCount;
            void* const* src = m_componentSetup.m_emptySentinals.data();

            for (wIndex i = 0; i < m_currentComponentTypeCount; ++i)
            {
                *dst++ = ComponentSetup::ListHeader::Filled(src[i]);
            }

            m_componentLists = static_cast<ComponentSetup::ListHeader*>(
                ::operator new(newCapacity * sceneSizeBytes, std::align_val_t(alignof(ComponentSetup::ListHeader)))
            );
        }
    }

    void ComponentSystem::DeleteSceneContent(std::size_t sceneStartListIndex)
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
    }
}